#include <stdio.h>
#include "window.h"
#include <stdlib.h>
#include <glad/glad.h>
#include <tinycthread.h>

static GLfloat vertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
};

static const char* vshdr_src = "\
#version 330 core \n\
layout (location=0) in vec3 position; \n\
void main() \n\
{ \n\
    gl_Position = vec4(position.xyz, 1.0); \n\
}";

static const char* fshdr_src = "\
#version 330 core \n\
out vec4 color; \n\
void main() \n\
{ \n\
    color = vec4(0.8, 0.0, 0.0, 1.0); \n\
}";

/* --------------------------------------------------
 * Checks and shows last shader compile error occurred
 * -------------------------------------------------- */
void gl_check_last_compile_error(GLuint id)
{
    /* Check if last compile was successful */
    GLint compileStatus;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE) {
        /* Gather the compile log size */
        GLint logLength;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength != 0) {
            /* Fetch and print log */
            GLchar* buf = malloc(logLength * sizeof(GLchar));
            glGetShaderInfoLog(id, logLength, 0, buf);
            fprintf(stderr, "Shader error: %s", buf);
            free(buf);
        }
    }
}

/* --------------------------------------------------
 * Checks and shows last shader link error occurred
 * -------------------------------------------------- */
void gl_check_last_link_error(GLuint id)
{
    /* Check if last link was successful */
    GLint status;
    glGetProgramiv(id, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        /* Gather the link log size */
        GLint logLength;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength != 0) {
            /* Fetch and print log */
            GLchar* buf = malloc(logLength * sizeof(GLchar));
            glGetProgramInfoLog(id, logLength, 0, buf);
            fprintf(stderr, "Shader program error: %s", buf);
            free(buf);
        }
    }
}

static void renderer(void* userdata)
{
    (void) userdata;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
    glEnableVertexAttribArray(0);

    GLuint vsh;
    vsh = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vsh, 1, &vshdr_src, 0);
    glCompileShader(vsh);

    GLuint fsh;
    fsh = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fsh, 1, &fshdr_src, 0);
    glCompileShader(fsh);

    GLuint shprog;
    shprog = glCreateProgram();
    glAttachShader(shprog, vsh);
    glAttachShader(shprog, fsh);
    glLinkProgram(shprog);

    glUseProgram(shprog);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glUseProgram(0);

    glDeleteShader(fsh);
    glDeleteShader(vsh);
    glDeleteProgram(shprog);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vbo);
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);

    if (glGetError() != GL_NO_ERROR) {
        printf("OpenGL error!\n");
    }
}

static int window_thrd(void* arg)
{
    struct window* wnd = (struct window*) arg;
    window_open(wnd);
    window_loop(wnd);
    return 0;
}

int main(int argc, char* argv[])
{
    (void) argc;
    (void) argv;

    /* Open window */
    struct window wnd;
    wnd.progress = 57;
    wnd.renderer = renderer;

    /* Launch window loop thread */
    thrd_t wnd_thrd;
    thrd_create(&wnd_thrd, window_thrd, &wnd);

    /* Wait for window loop thread to finish */
    thrd_join(wnd_thrd, 0);
    return 0;
}
