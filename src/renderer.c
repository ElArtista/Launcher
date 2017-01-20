#include "renderer.h"
#include <stdlib.h>
#include <stdio.h>
#include <glad/glad.h>

/* Convenience macro */
#define GLSRC(src) "#version 330 core\n" #src

static const char* vshdr_src = GLSRC(
    layout (location=0) in vec3 position;
    void main()
    {
        gl_Position = vec4(position.xyz, 1.0);
    }
);

static const char* fshdr_src = GLSRC(
    out vec4 f_color;
    uniform vec2 u_resolution;

    float smootherstep(float edge0, float edge1, float x)
    {
        float t = (x - edge0) / (edge1 - edge0);
        float t1 = t*t*t*(t*(t*6.0 - 15.0) + 10.0);
        return clamp(t1, 0.0, 1.0);
    }

    float df_disk(vec2 st, vec2 c, float r)
    {
        float ld = length(st - c);
        float bias = fwidth(ld);
        return (1.0 - smootherstep(r - bias, r + bias, ld));
    }

    void main()
    {
        vec2 st = 2.0 * (gl_FragCoord.xy - 0.5 * u_resolution.xy) / u_resolution.y;
        vec4 color = vec4(0.0);
        color = mix(color, vec4(1.0, 0.0, 0.0, 0.6), df_disk(st, vec2(0.0), 0.5));
        f_color = color;
    }
);

static void gl_check_last_compile_error(GLuint id)
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
            exit(EXIT_FAILURE);
        }
    }
}

static void gl_check_last_link_error(GLuint id)
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
            exit(EXIT_FAILURE);
        }
    }
}

static void render_quad()
{
    GLfloat quad_vert[] =
    {
       -1.0f,  1.0f, 0.0f,
       -1.0f, -1.0f, 0.0f,
        1.0f,  1.0f, 0.0f,
        1.0f, -1.0f, 0.0f
    };

    GLuint quad_vao;
    glGenVertexArrays(1, &quad_vao);
    glBindVertexArray(quad_vao);

    GLuint quad_vbo;
    glGenBuffers(1, &quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vert), &quad_vert, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &quad_vbo);
    glDeleteVertexArrays(1, &quad_vao);
}

void renderer_init(struct renderer_state* rs)
{
    GLuint vsh;
    vsh = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vsh, 1, &vshdr_src, 0);
    glCompileShader(vsh);
    gl_check_last_compile_error(vsh);

    GLuint fsh;
    fsh = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fsh, 1, &fshdr_src, 0);
    glCompileShader(fsh);
    gl_check_last_compile_error(fsh);

    GLuint shdr;
    shdr = glCreateProgram();
    glAttachShader(shdr, vsh);
    glAttachShader(shdr, fsh);
    glLinkProgram(shdr);
    gl_check_last_link_error(shdr);

    glDeleteShader(fsh);
    glDeleteShader(vsh);

    rs->shdr = shdr;
}

void renderer_render(struct renderer_state* rs)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    glUseProgram(rs->shdr);
    glUniform2f(glGetUniformLocation(rs->shdr, "u_resolution"), viewport[2], viewport[3]);
    render_quad();
    glUseProgram(0);
}

void renderer_destroy(struct renderer_state* rs)
{
    glUseProgram(0);
    glDeleteProgram(rs->shdr);
}
