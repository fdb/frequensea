// NDBX OpenGL Utility functions

#include <stdio.h>
#include <stdlib.h>

#include "ngl.h"

void ngl_check_gl_error(const char *file, int line) {
    GLenum err = glGetError();
    int has_error = 0;
    while (err != GL_NO_ERROR) {
        has_error = 1;
        char *msg = NULL;
        switch(err) {
            case GL_INVALID_OPERATION:
            msg = "GL_INVALID_OPERATION";
            break;
            case GL_INVALID_ENUM:
            msg = "GL_INVALID_ENUM";
            fprintf(stderr, "OpenGL error: GL_INVALID_ENUM\n");
            break;
            case GL_INVALID_VALUE:
            msg = "GL_INVALID_VALUE";
            fprintf(stderr, "OpenGL error: GL_INVALID_VALUE\n");
            break;
            case GL_OUT_OF_MEMORY:
            msg = "GL_OUT_OF_MEMORY";
            fprintf(stderr, "OpenGL error: GL_OUT_OF_MEMORY\n");
            break;
            default:
            msg = "UNKNOWN_ERROR";
        }
        fprintf(stderr, "OpenGL error: %s - %s:%d\n", msg, file, line);
        err = glGetError();
    }
    if (has_error) {
        exit(-1);
    }
}

void ngl_check_compile_error(GLuint shader) {
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        GLchar *strInfoLog = calloc(sizeof(GLchar), infoLogLength + 1);
        glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);
        fprintf(stderr, "Shader compile error: %s\n", strInfoLog);
        free(strInfoLog);
        exit(-1);
    }
}

void ngl_check_link_error(GLuint program) {
    int status = 0;
    glGetProgramiv (program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        GLchar *strInfoLog = calloc(sizeof(GLchar), infoLogLength + 1);
        glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
        fprintf(stderr, "Shader link error: %s\n", strInfoLog);
        free(strInfoLog);
        exit(-1);
    }
}

GLuint ngl_create_shader(const char *vertex_shader_source, const char *fragment_shader_source) {
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    ngl_check_compile_error(vertex_shader);
    NGL_CHECK_ERROR();

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    ngl_check_compile_error(fragment_shader);
    NGL_CHECK_ERROR();

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    ngl_check_link_error(program);
    NGL_CHECK_ERROR();

    return program;
}

void ngl_delete_shader(GLuint program) {
    glDeleteProgram(program);
    NGL_CHECK_ERROR();
}
