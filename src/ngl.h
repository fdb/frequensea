// NDBX OpenGL Utility functions

#ifndef NGL_H
#define NGL_H

#ifdef __APPLE__
#   define GLFW_INCLUDE_GLCOREARB
#else
    #include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>

#include "vec.h"
#include "nul.h"

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
    GLuint uv_vbo;
    GLuint vao;
    mat4 transform;
} ngl_model;

typedef struct {
    GLenum draw_mode;
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint program;
    GLint time_uniform;
    GLint view_matrix_uniform;
    GLint projection_matrix_uniform;
} ngl_shader;

typedef struct {
    ngl_shader *shader;
    GLuint texture_id;
} ngl_texture;

typedef struct {
    mat4 view;
    mat4 projection;
    ngl_color background;
} ngl_camera;

typedef struct {
    GLuint vbo;
    GLuint vao;
    GLuint texture;
    ngl_shader *shader;
} ngl_skybox;

void ngl_check_gl_error(const char *file, int line);
#define NGL_CHECK_ERROR() ngl_check_gl_error(__FILE__, __LINE__)
ngl_color ngl_color_init_rgba(float red, float green, float blue, float alpha);
void ngl_clear(float red, float green, float blue, float alpha);
void ngl_check_compile_error(GLuint shader);
void ngl_check_link_error(GLuint program);
ngl_shader *ngl_shader_new(GLenum draw_mode, const char *vertex_shader_source, const char *fragment_shader_source);
ngl_shader *ngl_shader_new_from_file(GLenum draw_mode, const char *vertex_fname, const char *fragment_fname);
void ngl_shader_free(ngl_shader *shader);
ngl_texture *ngl_texture_new(ngl_shader *shader, const char *uniform_name);
ngl_texture *ngl_texture_new_from_file(const char *file_name, ngl_shader *shader, const char *uniform_name);
void ngl_texture_update(ngl_texture *texture, nul_buffer *buffer, int width, int height);
void ngl_texture_free(ngl_texture *texture);
ngl_model* ngl_model_new(int component_count, int point_count, float* positions, float* normals, float* uvs);
ngl_model* ngl_model_new_grid_points(int row_count, int column_count, float row_height, float column_width);
ngl_model* ngl_model_new_grid_triangles(int row_count, int column_count, float row_height, float column_width);
ngl_model* ngl_model_new_with_height_map(int row_count, int column_count, float row_height, float column_width, float height_multiplier, int stride, int offset, const float *buffer);
ngl_model* ngl_model_load_obj(const char* fname);
void ngl_model_translate(ngl_model *model, float tx, float ty, float tz);
void ngl_model_free(ngl_model *model);
ngl_camera* ngl_camera_new_look_at(float x, float y, float z);
void ngl_camera_free(ngl_camera *camera);
ngl_skybox *ngl_skybox_new(const char *front, const char *back, const char *top, const char *bottom, const char *left, const char *right);
void ngl_skybox_draw(ngl_skybox *skybox, ngl_camera *camera);
void ngl_skybox_free(ngl_skybox *skybox);
void ngl_draw_model(ngl_camera *camera, ngl_model* model, ngl_shader *shader);

#endif // NGL_H
