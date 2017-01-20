#include "plat.h"
#ifdef OS_WINDOWS
#include "window.h"
#include <windows.h>
#include <stdlib.h>
#include <assert.h>
#include <glad/glad.h>
#include "wglctx.h"
#include <stdio.h>

/*----------------------------------------------------------------------
 * Platform specific data
 *----------------------------------------------------------------------*/
/* Window positioning state */
struct wnd_pos_state
{
    int is_moving;
    POINT mpos;
};

/* Window internal data */
struct wnd_internal
{
    /* Main state */
    HWND hwnd;       /* The window handle */
    /* Positioning state */
    struct wnd_pos_state ps;
    /* Rendering window */
    HWND rhwnd;
    HDC rhdc;        /* The window device context */
    HGLRC context;   /* The opengl context handle */
};

/*----------------------------------------------------------------------
 * Rendering window
 *----------------------------------------------------------------------*/
/* The window class unique identifier */
static const char* rendering_window_class_name = "rendering_window";
static const char* rendering_window_name = "";

static LRESULT CALLBACK rendering_window_callback_func(HWND hh, UINT mm, WPARAM ww, LPARAM ll)
{
    switch (mm) {
        case WM_SIZE: {
            int w = LOWORD(ll);
            int h = HIWORD(ll);
            glViewport(0, 0, w, h);
            break;
        }
        default:
            return DefWindowProc(hh, mm, ww, ll);
    }
    return 0;
}

static void register_rendering_window_class()
{
    /* The window class information structure */
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(WNDCLASSEX));

    /* Fill class info structure */
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.hInstance = GetModuleHandle(0);
    wc.lpfnWndProc = rendering_window_callback_func;
    wc.lpszClassName = rendering_window_class_name;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = 0;

    /* Register the window class */
    RegisterClassEx(&wc);
}

static HWND create_rendering_window()
{
    /* The window handle */
    HWND hwnd;

    /* Register the window class */
    register_rendering_window_class();

    /* Styles */
    int style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    int exstyle = WS_EX_APPWINDOW;

    /* Create the Window */
    hwnd = CreateWindowExA(
        exstyle,                     /* dwExStyle    */
        rendering_window_class_name, /* lpClassName  */
        rendering_window_name,       /* lpWindowName */
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
 * Actual window
 *----------------------------------------------------------------------*/
/* The window class unique identifier */
static const char* window_class_name = "launcher_window";

static LRESULT CALLBACK window_callback_func(HWND hh, UINT mm, WPARAM ww, LPARAM ll)
{
    struct window* wnd = (struct window*) GetWindowLongPtr(hh, GWLP_USERDATA);
    switch (mm) {
        case WM_CREATE: {
            struct window* window = (struct window*) ((CREATESTRUCT*)ll)->lpCreateParams;
            SetWindowLongPtr(hh, GWLP_USERDATA, (LONG_PTR) window);

            /* Remove borders and stuff */
            /*
            SetWindowLongPtr(hh, GWL_STYLE, 0);
            ShowWindow(hh, SW_SHOW);
            UpdateWindow(hh);
            */

            break;
        }
        case WM_LBUTTONDOWN: {
            if ((GetKeyState(VK_LMENU) & 0x8000) && (ww & MK_LBUTTON)) {
                wnd->internal->ps.is_moving = 1;
                SetCapture(hh);
                POINT cursor;
                memset(&cursor, 0, sizeof(POINT));
                GetCursorPos(&cursor);
                wnd->internal->ps.mpos = cursor;
            }
            break;
        }
        case WM_LBUTTONUP: {
            wnd->internal->ps.is_moving = 0;
            ReleaseCapture();
            memset(&wnd->internal->ps.mpos, 0, sizeof(POINT));
            break;
        }
        case WM_MOUSEMOVE: {
            if (wnd->internal->ps.is_moving) {
                POINT prevpos = wnd->internal->ps.mpos;
                POINT curpos;
                memset(&curpos, 0, sizeof(POINT));
                GetCursorPos(&curpos);

                RECT wrect;
                memset(&wrect, 0, sizeof(RECT));
                GetWindowRect(hh, &wrect);

                int xDiff = curpos.x - prevpos.x;
                int yDiff = curpos.y - prevpos.y;

                SetWindowPos(hh, NULL,
                    wrect.left + xDiff,
                    wrect.top + yDiff,
                    0, 0, SWP_NOSIZE | SWP_SHOWWINDOW
                );
                wnd->internal->ps.mpos = curpos;
            }
            break;
        }
        case WM_KEYDOWN: {
            if (ww == VK_ESCAPE)
                wnd->should_close = 1;
            break;
        }
        case WM_KEYUP: {
            switch (ww) {
                case VK_LMENU: {
                    wnd->internal->ps.is_moving = 0;
                    ReleaseCapture();
                    memset(&wnd->internal->ps.mpos, 0, sizeof(POINT));
                    break;
                }
            }
            break;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_SIZE: {
            int nw = LOWORD(ll);
            int nh = HIWORD(ll);
            RECT rect = { 0, 0, nw, nh };
            AdjustWindowRectEx(
                &rect,
                GetWindowLongPtr(wnd->internal->rhwnd, GWL_STYLE),
                0,
                GetWindowLongPtr(wnd->internal->rhwnd, GWL_EXSTYLE));
            SetWindowPos(wnd->internal->rhwnd, 0, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOREPOSITION | SWP_NOZORDER);
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hh, &ps);

            /* Gather window dimensions */
            RECT rect;
            GetWindowRect(hh, &rect);
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;

            /* Create memory DC */
            HDC hdc_mem = CreateCompatibleDC(0);

            /* Create DIB */
            void* pdata;
            BITMAPINFOHEADER bih;
            memset(&bih, 0, sizeof(BITMAPINFOHEADER));
            bih.biSize = sizeof(BITMAPINFOHEADER);
            bih.biWidth = width;
            bih.biHeight = height;
            bih.biPlanes = 1;
            bih.biBitCount = 32;
            bih.biCompression = BI_RGB;
            HBITMAP hbmp = CreateDIBSection(hdc_mem, (BITMAPINFO*)&bih, DIB_RGB_COLORS, &pdata, 0, 0);
            SelectObject(hdc_mem, hbmp);

            /* Gather image from GPU into temporary buffer */
            unsigned char* tmp_img = malloc(width * height * 4);
            memset(tmp_img, 0, width * height * 4);
            glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, tmp_img);
            /* Copy it to DIB flipped */
            for (int i = 0; i <  height; ++i) {
                for (int j = 0; j < width; ++j) {
                    unsigned char* src = (unsigned char*)tmp_img + ((height - i) * width + j) * 4;
                    unsigned char* dst = (unsigned char*)pdata + (i * width + j) * 4;
                    memcpy(dst, src, 4);
                }
            }
            /* Free temporary buffer */
            free(tmp_img);

            /* Blend transparent window */
            POINT pt_origin = { 0, 0 };
            SIZE sz = { width, height };
            BLENDFUNCTION blend;
            memset(&blend, 0, sizeof(BLENDFUNCTION));
            blend.BlendOp = AC_SRC_OVER;
            blend.SourceConstantAlpha = 255;
            blend.AlphaFormat = AC_SRC_ALPHA;
            UpdateLayeredWindow(hh, hdc, 0, &sz, hdc_mem, &pt_origin, RGB(0, 0, 0), &blend, ULW_ALPHA);

            /* Free resources */
            DeleteDC(hdc_mem);
            DeleteObject(hbmp);
            ReleaseDC(hh, hdc);
            EndPaint(hh, &ps);
            break;
        }
        case WM_CLOSE: {
            wnd->should_close = 1;
            break;
        }
        default:
            return DefWindowProc(hh, mm, ww, ll);
    }
    return 0;
}

static void register_window_class()
{
    /* The window class information structure */
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(WNDCLASSEX));

    /* Fill class info structure */
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = window_callback_func;
    wc.lpszClassName = window_class_name;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = 0;

    /* Register the window class */
    RegisterClassEx(&wc);
}

static HWND create_window(struct window* wnd)
{
    /* The window handle */
    HWND hwnd;

    /* Register the window class */
    register_window_class();

    /* Create the Window */
    hwnd = CreateWindowExA(
        WS_EX_LAYERED,         /* dwExStyle    */
        window_class_name,     /* lpClassName  */
        wnd->title,            /* lpWindowName */
        WS_POPUP | WS_VISIBLE, /* dwStyle      */
        CW_USEDEFAULT,         /* x            */
        CW_USEDEFAULT,         /* y            */
        wnd->width,            /* nWidth       */
        wnd->height,           /* nHeight      */
        0,                     /* nWndParent   */
        0,                     /* nMenu        */
        GetModuleHandle(0),    /* hInstance    */
        wnd                    /* lpParam      */
    );

    assert(hwnd);
    return hwnd;
}

static void poll_window_events(struct window* window)
{
    /* Window message var */
    MSG msg;
    /* Peek message loop, poll for events */
    while(PeekMessage(&msg, window->internal->hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    while(PeekMessage(&msg, window->internal->rhwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static void window_release(struct window* wnd)
{
    /* Release rendering window resources */
    DestroyWindow(wnd->internal->hwnd);
    wglMakeCurrent(wnd->internal->rhdc, 0);
    wglDeleteContext(wnd->internal->context);
    ReleaseDC(wnd->internal->rhwnd, wnd->internal->rhdc);
    DestroyWindow(wnd->internal->rhwnd);
    /* Release internal data struct */
    free(wnd->internal);
    wnd->internal = 0;
}

static long long get_timer_value()
{
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return count.QuadPart;
}

static void sleep(long msec)
{
    Sleep(msec);
}

#define FPS 25
typedef long long time_val_t;

void window_loop(struct window* wnd)
{
    time_val_t t1 = get_timer_value();
    while(!wnd->should_close) {
        time_val_t t2 = get_timer_value();
        if (t2 - t1 >= 1000 / FPS) {
            wnd->render_fn(wnd->render_data);
            SwapBuffers(wnd->internal->rhdc);
            UpdateWindow(wnd->internal->hwnd);
            RedrawWindow(wnd->internal->hwnd, 0, 0, RDW_ERASE | RDW_INTERNALPAINT | RDW_INVALIDATE);
            poll_window_events(wnd);
        }
        else
            sleep((long)((1000 / FPS) - (t2 - t1)));
        t1 = t2;
    }
    window_release(wnd);
}

void window_open(struct window* window)
{
    /* Create internal struct */
    window->internal = calloc(1, sizeof(struct wnd_internal));

    /* Create rendering winodw instance */
    window->internal->rhwnd = create_rendering_window();

    /* Create opengl context */
    create_wgl_context(
        window->internal->rhwnd,
        &window->internal->rhdc,
        &window->internal->context
    );

    /* Create the window instance */
    window->internal->hwnd = create_window(window);

    /* Set the close flag */
    window->should_close = 0;
}
#endif
