#include "renderer.h"
#include <glad/glad.h>

/* Convenience macro */
#define GLSRC(src) "#version 330 core\n" #src

static const GLfloat vertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
};

static const char* vshdr_src = GLSRC(
    layout (location=0) in vec3 position;
    void main()
    {
        gl_Position = vec4(position.xyz, 1.0);
    }
);

static const char* fshdr_src = GLSRC(
    out vec4 color;
    void main()
    {
        color = vec4(0.8, 0.0, 0.0, 1.0);
    }
);

void renderer(void* userdata)
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
}
