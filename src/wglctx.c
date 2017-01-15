#ifdef _WIN32
#include "wglctx.h"
#include <assert.h>
#include <stdio.h>
#include <glad/glad.h>
#include <wglext.h>

struct wgl_extension_funcs
{
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
    PFNWGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribivARB;
    PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT;
    PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
};

/*----------------------------------------------------------------------
 * Debug callback
 *----------------------------------------------------------------------*/
static void APIENTRY gl_debug_output(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar *message,
    const void *userparam)
{
    (void) length;
    (void) userparam;

    /* Ignore non-significant error/warning codes */
    if(type != GL_DEBUG_TYPE_ERROR)
        return;

    const char* s = 0;
    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             s = "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   s = "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: s = "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     s = "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     s = "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           s = "Source: Other"; break;
        default:                              s = "???"; break;
    }

    const char* t = 0;
    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               t = "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: t = "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  t = "Type: Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         t = "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         t = "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              t = "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          t = "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           t = "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               t = "Type: Other"; break;
        default:                                t = "???"; break;
    }

    const char* sv = 0;
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         sv = "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       sv = "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          sv = "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: sv = "Severity: notification"; break;
        default:                             sv = "???"; break;
    }

    printf("Debug message (%d): %s\n", id, message);
    printf("%s\n%s\n%s\n", s, t, sv);
    assert(0);
}

/*----------------------------------------------------------------------
 * Temp Context window
 *----------------------------------------------------------------------*/
static const char* temp_context_wnd_class = "temp_context_wnd_class";

static LRESULT CALLBACK dummy_wndproc(HWND hh, UINT mm, WPARAM ww, LPARAM ll)
{
    return DefWindowProc(hh, mm, ww, ll);
}

static void register_temp_context_window_class()
{
    /* The window class information structure */
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(WNDCLASSEX));

    /* Fill class info structure */
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.hInstance = GetModuleHandle(0);
    wc.lpfnWndProc = dummy_wndproc;
    wc.lpszClassName = temp_context_wnd_class;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = 0;

    /* Register the window class */
    RegisterClassEx(&wc);
}

static HWND create_temp_context_window()
{
    /* The window handle */
    HWND hwnd;

    /* Register the window class */
    register_temp_context_window_class();

    /* Styles */
    int style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    int exstyle = WS_EX_APPWINDOW;

    /* Create the Window */
    hwnd = CreateWindowExA(
        exstyle,                     /* dwExStyle    */
        temp_context_wnd_class,      /* lpClassName  */
        "",                          /* lpWindowName */
        style,                       /* dwStyle      */
        CW_USEDEFAULT,               /* x            */
        CW_USEDEFAULT,               /* y            */
        0,                           /* nWidth       */
        0,                           /* nHeight      */
        0,                           /* nWndParent   */
        0,                           /* nMenu        */
        GetModuleHandle(0),          /* hInstance    */
        0                            /* lpParam      */
    );

    assert(hwnd);
    return hwnd;
}

/*----------------------------------------------------------------------
 * Context initialization
 *----------------------------------------------------------------------*/
static void initialize_opengl_extensions(struct wgl_extension_funcs* funcs)
{
    /* Handle to temporary window */
    /*
     * This is a necessity, because there are problems when calling
     * SetPixelFormat to the same window multiple times
     */
    HWND hwnd = create_temp_context_window();

    /* Handle to device context */
    HDC hdc;

    /* Pixel format hint struct */
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

    /* Fill in the struct */
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA; /* The kind of framebuffer. RGBA or palette */
    pfd.cColorBits = 32;            /* Colordepth of the framebuffer */
    pfd.cDepthBits = 24;            /* Number of bits for the depthbuffer */
    pfd.cStencilBits = 8;           /* Number of bits for the stencilbuffer */
    pfd.iLayerType = PFD_MAIN_PLANE;

    /* Get the Device Context */
    hdc = GetDC(hwnd);

    /* Query for a matching pixel format */
    int pixel_fmt = ChoosePixelFormat(hdc, &pfd);

    /* Set window's pixel format to the above */
    SetPixelFormat(hdc, pixel_fmt, &pfd);

    /* Create opengl context and make it current */
    HGLRC temp_context = (HGLRC) wglCreateContext(hdc);
    wglMakeCurrent(hdc, temp_context);

    /* Load extension funcs */
    funcs->wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");
    funcs->wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
    funcs->wglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC) wglGetProcAddress("wglGetPixelFormatAttribivARB");
    funcs->wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC) wglGetProcAddress("wglGetExtensionsStringEXT");
    funcs->wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC) wglGetProcAddress("wglGetExtensionsStringARB");
    funcs->wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC) wglGetProcAddress("wglChoosePixelFormatARB");

    /* Release temporary context resources */
    wglMakeCurrent(hdc, 0);
    wglDeleteContext(temp_context);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
}

void create_wgl_context(HWND hwnd, HDC* hdc, HGLRC* hglrc)
{
    /* Holder for the wgl extension functions */
    struct wgl_extension_funcs wgl;
    memset(&wgl, 0, sizeof(struct wgl_extension_funcs));

    /* Creates a temp context in order to load opengl functions */
    initialize_opengl_extensions(&wgl);

    /* Get the Device Context */
    *hdc = GetDC(hwnd);

    /* Choose pixel format using extension */
    int pixel_fmts[25];
    memset(pixel_fmts, 0, sizeof(pixel_fmts));
    UINT num_fmts = 0;
    const int attrib_list[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 24,
        WGL_ALPHA_BITS_ARB, 8,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        0
    };
    wgl.wglChoosePixelFormatARB(*hdc, attrib_list, 0, 25, pixel_fmts, &num_fmts);

    /* Populate pfd struct in order to set pixel format */
    BOOL pixel_fmt_set = FALSE;
    for (unsigned int i = 0; i < num_fmts; ++i) {
        PIXELFORMATDESCRIPTOR pfd;
        DescribePixelFormat(*hdc, pixel_fmts[i], sizeof(pfd), &pfd);

        /* Set newly choosen pixel format */
        pixel_fmt_set = SetPixelFormat(*hdc, pixel_fmts[i], &pfd);
        if (pixel_fmt_set == TRUE)
            break;
    }
    assert(pixel_fmt_set);

    /* Create context using extension and make it current */
    int ctx_attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };
    *hglrc = (HGLRC) wgl.wglCreateContextAttribsARB(*hdc, 0, ctx_attribs);
    wglMakeCurrent(*hdc, *hglrc);

    /* Load core profile functions */
    gladLoadGL();

    /* Register debug callback */
    GLint flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_debug_output, 0);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
    }
}
#endif
