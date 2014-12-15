// NDBX OpenGL Utility functions

#include <stdio.h>
#include <stdlib.h>

#include "nfile.h"
#include "obj.h"

#include "ngl.h"

// Error checking ////////////////////////////////////////////////////////////

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

// Background ////////////////////////////////////////////////////////////////

void ngl_clear(float red, float green, float blue, float alpha) {
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// Shaders ///////////////////////////////////////////////////////////////////

void ngl_check_compile_error(GLuint shader) {
    const int LOG_MAX_LENGTH = 2048;
    GLint status = -1;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char infoLog[LOG_MAX_LENGTH];
        glGetShaderInfoLog(shader, LOG_MAX_LENGTH, NULL, infoLog);
        fprintf(stderr, "Shader %d compile error: %s\n", shader, infoLog);
        exit(-1);
    }
}

void ngl_check_link_error(GLuint program) {
    const int LOG_MAX_LENGTH = 2048;
    GLint status = -1;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        char infoLog[LOG_MAX_LENGTH];
        glGetProgramInfoLog(program, LOG_MAX_LENGTH, NULL, infoLog);
        fprintf(stderr, "Shader link error: %s\n", infoLog);
        exit(-1);
    }
}

ngl_shader *ngl_create_shader(const char *vertex_shader_source, const char *fragment_shader_source) {
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

    ngl_shader *shader = malloc(sizeof(ngl_shader));
    shader->program = program;
    shader->time_uniform = glGetUniformLocation(program, "uTime");
    shader->view_matrix_uniform = glGetUniformLocation(program, "uViewMatrix");
    shader->projection_matrix_uniform = glGetUniformLocation(program, "uProjectionMatrix");

    return shader;
}

ngl_shader *ngl_load_shader(const char *vertex_fname, const char *fragment_fname) {
    char *vertex_shader_source = nfile_read(vertex_fname);
    char *fragment_shader_source = nfile_read(fragment_fname);
    ngl_shader *shader = ngl_create_shader(vertex_shader_source, fragment_shader_source);
    free(vertex_shader_source);
    free(fragment_shader_source);
    return shader;
}

void ngl_delete_shader(ngl_shader *shader) {
    glDeleteProgram(shader->program);
    NGL_CHECK_ERROR();
    free(shader);
}

// OBJ Models ////////////////////////////////////////////////////////////////

ngl_model* ngl_load_obj(const char* fname) {
    ngl_model *model = malloc(sizeof(ngl_model));

    mat4 identity = mat4_init_identity();
    mat4 *transform = malloc(sizeof(mat4));
    mat4_set(transform, &identity);
    model->transform = transform;

    static float *points;
    static float *normals;
    obj_parse(fname, &points, &normals, &model->face_count);
    // Each triangle face has 3 vertices
    int point_count = model->face_count * 3;

    glGenBuffers(1, &model->position_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, model->position_vbo);
    glBufferData(GL_ARRAY_BUFFER, point_count * 3 * sizeof(GLfloat), points, GL_DYNAMIC_DRAW);
    NGL_CHECK_ERROR();

    glGenBuffers(1, &model->normal_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, model->normal_vbo);
    glBufferData(GL_ARRAY_BUFFER, point_count * 3 * sizeof(GLfloat), normals, GL_DYNAMIC_DRAW);
    NGL_CHECK_ERROR();

    // Vertex array object
    // FIXME glUseProgram?
    glGenVertexArrays(1, &model->vao);
    glBindVertexArray(model->vao);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, model->position_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, model->normal_vbo);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    NGL_CHECK_ERROR();

    return model;
}

// Model drawing /////////////////////////////////////////////////////////////

void ngl_draw_model(ngl_model* model, ngl_shader *shader) {
    vec3 camera = vec3_init(-20.0f, 18.0f, 50.0f);
    vec3 target = vec3_zero();
    vec3 up = vec3_init(0.0f, 1.0f, 0.0f);
    mat4 v = mat4_init_look_at(&camera, &target, &up);
    mat4 p = mat4_init_perspective(67, 800 / 600, 0.01f, 1000.0f);
    mat4 mv = mat4_mul(model->transform, &v);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glUseProgram(shader->program);
    NGL_CHECK_ERROR();
    // glUniform1f(shader->time_uniform, time);
    // NGL_CHECK_ERROR();
    glUniformMatrix4fv(shader->view_matrix_uniform, 1, GL_FALSE, (GLfloat *)&mv.m);
    NGL_CHECK_ERROR();
    glUniformMatrix4fv(shader->projection_matrix_uniform, 1, GL_FALSE, (GLfloat *)&p.m);
    NGL_CHECK_ERROR();
    glBindVertexArray(model->vao);
    NGL_CHECK_ERROR();
    glDrawArrays(GL_TRIANGLES, 0, model->face_count * 3);
    NGL_CHECK_ERROR();

    glBindVertexArray(0);
    NGL_CHECK_ERROR();
    glUseProgram(0);
    NGL_CHECK_ERROR();
}
