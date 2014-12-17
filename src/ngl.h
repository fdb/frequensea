// NDBX OpenGL Utility functions

#ifndef NGL_H
#define NGL_H

#ifdef __APPLE__
#   define GLFW_INCLUDE_GLCOREARB
#endif
#include <GLFW/glfw3.h>

#include "vec.h"

typedef struct {
    float red;
    float green;
    float blue;
    float alpha;
} ngl_color;

typedef struct {
    int point_count;
    GLuint position_vbo;
    GLuint normal_vbo;
    GLuint vao;
    mat4 transform;
} ngl_model;

typedef struct {
    GLenum draw_mode;
    GLuint program;
    GLint time_uniform;
    GLint view_matrix_uniform;
    GLint projection_matrix_uniform;
} ngl_shader;

typedef struct {
    mat4 transform;
    mat4 projection;
    ngl_color background;
} ngl_camera;

void ngl_check_gl_error(const char *file, int line);
#define NGL_CHECK_ERROR() ngl_check_gl_error(__FILE__, __LINE__)
ngl_color ngl_color_init_rgba(float red, float green, float blue, float alpha);
void ngl_clear(float red, float green, float blue, float alpha);
void ngl_check_compile_error(GLuint shader);
void ngl_check_link_error(GLuint program);
ngl_shader *ngl_create_shader(GLenum draw_mode, const char *vertex_shader_source, const char *fragment_shader_source);
ngl_shader *ngl_load_shader(GLenum draw_mode, const char *vertex_fname, const char *fragment_fname);
void ngl_delete_shader(ngl_shader *shader);
ngl_model* ngl_model_init_positions(int n_components, int n_points, float* positions);
ngl_model* ngl_load_obj(const char* fname);
void ngl_draw_model(ngl_camera *camera, ngl_model* model, ngl_shader *shader);

#endif // NGL_H
