// NDBX OpenGL Utility functions

#include <stdio.h>
#include <stdlib.h>

#include "nfile.h"
#include "obj.h"

#include "ngl.h"
#include "nwm.h"

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

// Color /////////////////////////////////////////////////////////////////////

ngl_color ngl_color_init_rgba(float red, float green, float blue, float alpha) {
    ngl_color c;
    c.red = red;
    c.green = green;
    c.blue = blue;
    c.alpha = alpha;
    return c;
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

ngl_shader *ngl_shader_init(GLenum draw_mode, const char *vertex_shader_source, const char *fragment_shader_source) {
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
    shader->draw_mode = draw_mode;
    shader->program = program;
    shader->time_uniform = glGetUniformLocation(program, "uTime");
    shader->view_matrix_uniform = glGetUniformLocation(program, "uViewMatrix");
    shader->projection_matrix_uniform = glGetUniformLocation(program, "uProjectionMatrix");

    return shader;
}

ngl_shader *ngl_load_shader(GLenum draw_mode, const char *vertex_fname, const char *fragment_fname) {
    char *vertex_shader_source = nfile_read(vertex_fname);
    char *fragment_shader_source = nfile_read(fragment_fname);
    ngl_shader *shader = ngl_shader_init(draw_mode, vertex_shader_source, fragment_shader_source);
    free(vertex_shader_source);
    free(fragment_shader_source);
    return shader;
}

void ngl_delete_shader(ngl_shader *shader) {
    glDeleteProgram(shader->program);
    NGL_CHECK_ERROR();
    free(shader);
}

// Textures //////////////////////////////////////////////////////////////////

ngl_texture *ngl_texture_create(ngl_shader *shader, const char *uniform_name) {
    ngl_texture *texture = malloc(sizeof(ngl_texture));
    texture->shader = shader;

    glGenTextures(1, &texture->texture_id);
    NGL_CHECK_ERROR();
    glActiveTexture(GL_TEXTURE0);
    NGL_CHECK_ERROR();
    glBindTexture(GL_TEXTURE_2D, texture->texture_id);
    NGL_CHECK_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    NGL_CHECK_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    NGL_CHECK_ERROR();

    GLuint u_texture = glGetUniformLocation(shader->program, uniform_name);
    NGL_CHECK_ERROR();
    if (u_texture == -1) {
        fprintf(stderr, "WARN OpenGL: Could not find uniform %s\n", uniform_name);
    }

    return texture;
}

// format: one of GL_RED, GL_RG, GL_RGB or GL_RGBA. Used both for internalFormat and format parameters.
// The values are assumed to be GL_FLOATs.

void ngl_texture_update(ngl_texture *texture, GLint format, GLsizei width, GLsizei height, const float *data) {
    glActiveTexture(GL_TEXTURE0);
    NGL_CHECK_ERROR();
    glBindTexture(GL_TEXTURE_2D, texture->texture_id);
    NGL_CHECK_ERROR();
    //printf("f %d w %d h %d d %.2f\n", format, width, height, data[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_FLOAT, data);
    NGL_CHECK_ERROR();
}

void ngl_texture_delete(ngl_texture *texture) {
    glDeleteTextures(1, &texture->texture_id);
    NGL_CHECK_ERROR();
    free(texture);
}

// Model initialization //////////////////////////////////////////////////////

ngl_model* ngl_model_init_positions(int component_count, int point_count, float* positions, float* normals, float* uvs) {
    ngl_model *model = malloc(sizeof(ngl_model));
    model->point_count = point_count;
    model->transform = mat4_init_identity();

    if (positions != NULL) {
        glGenBuffers(1, &model->position_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, model->position_vbo);
        glBufferData(GL_ARRAY_BUFFER, point_count * component_count * sizeof(GLfloat), positions, GL_DYNAMIC_DRAW);
        NGL_CHECK_ERROR();
    } else {
        model->position_vbo = -1;
    }

    if (normals != NULL) {
        glGenBuffers(1, &model->normal_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, model->normal_vbo);
        glBufferData(GL_ARRAY_BUFFER, point_count * component_count * sizeof(GLfloat), normals, GL_DYNAMIC_DRAW);
        NGL_CHECK_ERROR();
    } else {
        model->normal_vbo = -1;
    }

    if (uvs != NULL) {
        glGenBuffers(1, &model->uv_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, model->uv_vbo);
        glBufferData(GL_ARRAY_BUFFER, point_count * 2 * sizeof(GLfloat), uvs, GL_DYNAMIC_DRAW);
        NGL_CHECK_ERROR();
    } else {
        model->uv_vbo = -1;
    }

    glGenVertexArrays(1, &model->vao);
    glBindVertexArray(model->vao);

    if (positions != NULL) {
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, model->position_vbo);
        glVertexAttribPointer(0, component_count, GL_FLOAT, GL_FALSE, 0, NULL);
        NGL_CHECK_ERROR();
    }

    if (normals != NULL) {
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, model->normal_vbo);
        glVertexAttribPointer(1, component_count, GL_FLOAT, GL_FALSE, 0, NULL);
        NGL_CHECK_ERROR();
    }

    if (uvs != NULL) {
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, model->uv_vbo);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        NGL_CHECK_ERROR();
    }

    return model;
}

ngl_model* ngl_model_init_grid_points(int row_count, int column_count, float row_height, float column_width) {
    int point_count = row_count * column_count;
    float* points = malloc(point_count * 2 * sizeof(float));
    float total_width = (column_count - 1) * column_width;
    float total_height = (row_count - 1) * row_height;
    float left = - total_width / 2;
    float top = - total_height / 2;
    int i = 0;
    for (int y = 0; y < row_count; y++) {
        for (int x = 0; x < column_count; x++) {
            points[i++] = left + x * column_width;
            points[i++] = top + y * row_height;
        }
    }
    return ngl_model_init_positions(2, i / 2, points, NULL, NULL);
}

ngl_model* ngl_model_init_grid_triangles(int row_count, int column_count, float row_height, float column_width) {
    int square_count = (row_count - 1) * (column_count - 1);
    int face_count = square_count * 2;
    int point_count = face_count * 3;
    float* points = malloc(point_count * 3 * sizeof(float));
    float* normals = malloc(point_count * 3 * sizeof(float));
    float* uvs = malloc(point_count * 2 * sizeof(float));
    float total_width = (column_count - 1) * column_width;
    float total_height = (row_count - 1) * row_height;
    float left = - total_width / 2;
    float top = - total_height / 2;
    int pt_index = 0;
    int uv_index = 0;
    for (int ri = 0; ri < row_count - 1; ri++) {
        for (int ci = 0; ci < column_count - 1; ci++) {
            float x = left + ci * column_width;
            float z = top + ri * row_height;

            vec3 v11 = vec3_init(x, 0, z);
            vec3 v12 = vec3_init(x + column_width, 0, z);
            vec3 v21 = vec3_init(x, 0, z + row_height);
            vec3 v22 = vec3_init(x + column_width, 0, z + row_height);
            vec3 n1 = vec3_normal(&v11, &v12, &v21);
            vec3 n2 = vec3_normal(&v12, &v22, &v21);
            vec2 uv11 = vec2_init(ci / (float) (column_count - 1), ri / (float) (row_count - 1));
            vec2 uv12 = vec2_init((ci + 1) / (float) (column_count - 1), ri / (float) (row_count - 1));
            vec2 uv21 = vec2_init(ci / (float) (column_count - 1), (ri + 1) / (float) (row_count - 1));
            vec2 uv22 = vec2_init((ci + 1) / (float) (column_count - 1), (ri + 1) / (float) (row_count - 1));

            points[pt_index] = v11.x;
            points[pt_index + 1] = v11.y;
            points[pt_index + 2] = v11.z;
            normals[pt_index] = n1.x;
            normals[pt_index + 1] = n1.y;
            normals[pt_index + 2] = n1.z;
            uvs[uv_index] = uv11.x;
            uvs[uv_index + 1] = uv11.y;

            points[pt_index + 3] = v12.x;
            points[pt_index + 4] = v12.y;
            points[pt_index + 5] = v12.z;
            normals[pt_index + 3] = n1.x;
            normals[pt_index + 4] = n1.y;
            normals[pt_index + 5] = n1.z;
            uvs[uv_index + 2] = uv12.x;
            uvs[uv_index + 3] = uv12.y;

            points[pt_index + 6] = v21.x;
            points[pt_index + 7] = v21.y;
            points[pt_index + 8] = v21.z;
            normals[pt_index + 6] = n1.x;
            normals[pt_index + 7] = n1.y;
            normals[pt_index + 8] = n1.z;
            uvs[uv_index + 4] = uv21.x;
            uvs[uv_index + 5] = uv21.y;

            points[pt_index + 9] = v12.x;
            points[pt_index + 10] = v12.y;
            points[pt_index + 11] = v12.z;
            normals[pt_index + 9] = n2.x;
            normals[pt_index + 10] = n2.y;
            normals[pt_index + 11] = n2.z;
            uvs[uv_index + 6] = uv12.x;
            uvs[uv_index + 7] = uv12.y;

            points[pt_index + 12] = v22.x;
            points[pt_index + 13] = v22.y;
            points[pt_index + 14] = v22.z;
            normals[pt_index + 12] = n2.x;
            normals[pt_index + 13] = n2.y;
            normals[pt_index + 14] = n2.z;
            uvs[uv_index + 8] = uv22.x;
            uvs[uv_index + 9] = uv22.y;

            points[pt_index + 15] = v21.x;
            points[pt_index + 16] = v21.y;
            points[pt_index + 17] = v21.z;
            normals[pt_index + 15] = n2.x;
            normals[pt_index + 16] = n2.y;
            normals[pt_index + 17] = n2.z;
            uvs[uv_index + 10] = uv21.x;
            uvs[uv_index + 11] = uv21.y;

            pt_index += 18;
            uv_index += 12;
        }
    }
    return ngl_model_init_positions(3, point_count, points, normals, uvs);
}

ngl_model* ngl_load_obj(const char* fname) {
    ngl_model *model = malloc(sizeof(ngl_model));

    model->transform = mat4_init_identity();

    static float *points;
    static float *normals;
    int face_count;
    obj_parse(fname, &points, &normals, &face_count);
    // Each triangle face has 3 vertices
    model->point_count = face_count * 3;

    glGenBuffers(1, &model->position_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, model->position_vbo);
    glBufferData(GL_ARRAY_BUFFER, model->point_count * 3 * sizeof(GLfloat), points, GL_DYNAMIC_DRAW);
    NGL_CHECK_ERROR();

    glGenBuffers(1, &model->normal_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, model->normal_vbo);
    glBufferData(GL_ARRAY_BUFFER, model->point_count * 3 * sizeof(GLfloat), normals, GL_DYNAMIC_DRAW);
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

// Camera ////////////////////////////////////////////////////////////////////

ngl_camera* ngl_camera_init_look_at(float x, float y, float z) {
    ngl_camera *camera = malloc(sizeof(ngl_camera));
    vec3 loc = vec3_init(x, y, z);
    vec3 target = vec3_zero();
    vec3 up = vec3_init(0.0f, 1.0f, 0.0f);
    camera->transform = mat4_init_look_at(&loc, &target, &up);
    camera->projection = mat4_init_perspective(67, 800 / 600, 0.01f, 1000.0f);
    camera->background = ngl_color_init_rgba(0, 0, 1, 1);
    return camera;
}

// Model drawing /////////////////////////////////////////////////////////////

void ngl_draw_model(ngl_camera* camera, ngl_model* model, ngl_shader *shader) {
    mat4 view = camera->transform;
    mat4 projection = camera->projection;
    mat4 mv = mat4_mul(&model->transform, &view);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    NGL_CHECK_ERROR();
    glEnable(GL_BLEND);
    NGL_CHECK_ERROR();
    glUseProgram(shader->program);
    NGL_CHECK_ERROR();
    glUniform1f(shader->time_uniform, nwm_get_time());
    NGL_CHECK_ERROR();
    glUniformMatrix4fv(shader->view_matrix_uniform, 1, GL_FALSE, (GLfloat *)&mv.m);
    NGL_CHECK_ERROR();
    glUniformMatrix4fv(shader->projection_matrix_uniform, 1, GL_FALSE, (GLfloat *)&projection.m);
    NGL_CHECK_ERROR();

    glBindVertexArray(model->vao);
    NGL_CHECK_ERROR();
    glDrawArrays(shader->draw_mode, 0, model->point_count);
    NGL_CHECK_ERROR();

    glBindVertexArray(0);
    NGL_CHECK_ERROR();
    glUseProgram(0);
    NGL_CHECK_ERROR();
}
