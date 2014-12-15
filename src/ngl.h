// NDBX OpenGL Utility functions

#ifndef NGL_H
#define NGL_H

#ifdef __APPLE__
#   define GLFW_INCLUDE_GLCOREARB
#endif
#include <GLFW/glfw3.h>

void ngl_check_gl_error(const char *file, int line);
#define NGL_CHECK_ERROR() ngl_check_gl_error(__FILE__, __LINE__)

void ngl_check_compile_error(GLuint shader);
void ngl_check_link_error(GLuint program);
GLuint ngl_create_shader(const char *vertex_shader_source, const char *fragment_shader_source);
void ngl_delete_shader(GLuint program);

#endif // NGL_H
