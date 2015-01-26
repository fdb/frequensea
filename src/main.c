#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "ngl.h"
#include "nim.h"
#include "nrf.h"
#include "nvr.h"
#include "nwm.h"
#include "vec.h"
#include "nfile.h"

// Lua utility functions ////////////////////////////////////////////////////

static void l_register_type(lua_State *L, const char *type, lua_CFunction gc_fn) {
    luaL_newmetatable(L, type);
    if (gc_fn != NULL) {
        lua_pushcfunction(L, gc_fn);
        lua_setfield(L, -2, "__gc");
        lua_pop(L, 1);
    }
}

static void l_to_table(lua_State *L, const char *type, void *obj) {
    lua_newtable(L);
    luaL_getmetatable(L, type);
    lua_setmetatable(L, -2);
    lua_pushliteral(L, "__type__");
    lua_pushstring(L, type);
    lua_settable(L, -3);
    lua_pushliteral(L, "__ptr__");
    lua_pushlightuserdata(L, obj);
    lua_settable(L, -3);
}

static void* l_from_table(lua_State *L, const char *type, int index) {
    luaL_checktype(L, index, LUA_TTABLE);
    lua_pushliteral(L, "__type__");
    lua_gettable(L, index);
    const char *table_type = lua_tostring(L, -1);
    if (strcmp(type, table_type) == 0) {
        lua_pushliteral(L, "__ptr__");
        lua_gettable(L, index);
        void *data = lua_touserdata(L, -1);
        return data;
    } else {
        fprintf(stderr, "Lua: invalid type for param %d: expected %s, was %s\n", index, type, table_type);
        exit(EXIT_FAILURE);
    }
}

static ngl_camera* l_to_ngl_camera(lua_State *L, int index) {
    return (ngl_camera*) l_from_table(L, "ngl_camera", index);
}

static ngl_model* l_to_ngl_model(lua_State *L, int index) {
    return (ngl_model*) l_from_table(L, "ngl_model", index);
}

static ngl_shader* l_to_ngl_shader(lua_State *L, int index) {
    return (ngl_shader*) l_from_table(L, "ngl_shader", index);
}

static ngl_texture* l_to_ngl_texture(lua_State *L, int index) {
    return (ngl_texture*) l_from_table(L, "ngl_texture", index);
}

static nrf_device* l_to_nrf_device(lua_State *L, int index) {
    return (nrf_device*) l_from_table(L, "nrf_device", index);
}

static void l_register_function(lua_State *L, const char *name, lua_CFunction fn) {
    lua_pushcfunction(L, fn);
    lua_setglobal(L, name);
}

static void l_register_constant(lua_State *L, const char *name, int value) {
    lua_pushinteger(L, value);
    lua_setglobal(L, name);
}

static int l_call_function(lua_State *L, const char *name) {
    lua_getglobal(L, name);
    if (lua_isfunction(L, -1)) {
        int error = lua_pcall(L, 0, 0, 0);
        if (error) {
            fprintf(stderr, "Error calling %s(): %s\n", name, lua_tostring(L, -1));
            lua_pop(L, 1);
            return -1;
        } else {
            return 0;
        }
    } else {
        lua_pop(L, 1);
        fprintf(stderr, "WARN: No \"%s\" function.\n", name);
        return -1;
    }
}

// Lua NWM wrappers /////////////////////////////////////////////////////////

static int l_nwm_get_time(lua_State *L) {
    lua_pushnumber(L, nwm_get_time());
    return 1;
}

// Lua NGL wrappers /////////////////////////////////////////////////////////

static int l_ngl_clear(lua_State *L) {
    float red = luaL_checknumber(L, 1);
    float green = luaL_checknumber(L, 2);
    float blue = luaL_checknumber(L, 3);
    float alpha = luaL_checknumber(L, 4);
    ngl_clear(red, green, blue, alpha);
    return 0;
}

static int l_ngl_camera_new_look_at(lua_State *L) {
    float camera_x = luaL_checknumber(L, 1);
    float camera_y = luaL_checknumber(L, 2);
    float camera_z = luaL_checknumber(L, 3);
    ngl_camera *camera = ngl_camera_new_look_at(camera_x, camera_y, camera_z);
    l_to_table(L, "ngl_camera", camera);
    return 1;
}

static int l_ngl_camera_free(lua_State *L) {
    ngl_camera *camera = l_to_ngl_camera(L, 1);
    ngl_camera_free(camera);
    return 0;
}

static int l_ngl_shader_new(lua_State *L) {
    GLenum draw_mode = luaL_checkint(L, 1);
    const char *vertex_shader = lua_tostring(L, 2);
    const char *fragment_shader = lua_tostring(L, 3);

    ngl_shader *shader = ngl_shader_new(draw_mode, vertex_shader, fragment_shader);
    l_to_table(L, "ngl_shader", shader);
    return 1;
}

static int l_ngl_shader_new_from_file(lua_State *L) {
    GLenum draw_mode = luaL_checkint(L, 1);
    const char *vertex_fname = lua_tostring(L, 2);
    const char *fragment_fname = lua_tostring(L, 3);

    ngl_shader *shader = ngl_shader_new_from_file(draw_mode, vertex_fname, fragment_fname);
    l_to_table(L, "ngl_shader", shader);
    return 1;
}

static int l_ngl_shader_free(lua_State *L) {
    ngl_shader *shader = l_to_ngl_shader(L, 1);
    ngl_shader_free(shader);
    return 0;
}

static int l_ngl_texture_new(lua_State *L) {
    ngl_shader *shader = l_to_ngl_shader(L, 1);
    const char *uniform_name = lua_tostring(L, 2);
    ngl_texture *texture = ngl_texture_new(shader, uniform_name);
    l_to_table(L, "ngl_texture", texture);
    return 1;
}

static int l_ngl_texture_update(lua_State *L) {
    ngl_texture *texture = l_to_ngl_texture(L, 1);
    int format = luaL_checkint(L, 2);
    int width = luaL_checkint(L, 3);
    int height = luaL_checkint(L, 4);
    float *data = NULL;
    if (!lua_isnoneornil(L, 5)) {
        luaL_checkany(L, 5);
        data = (float *) lua_touserdata(L, 5);
    }
    ngl_texture_update(texture, format, width, height, data);
    return 0;
}

static int l_ngl_texture_free(lua_State *L) {
    ngl_texture *texture = l_to_ngl_texture(L, 1);
    free(texture);
    return 0;
}

static int l_ngl_model_new(lua_State *L) {
    int component_count = luaL_checkint(L, 1);
    int point_count = luaL_checkint(L, 2);
    float *positions = NULL;
    if (!lua_isnoneornil(L, 3)) {
        luaL_checkany(L, 3);
        positions = (float *) lua_touserdata(L, 3);
    }
    float *normals = NULL;
    if (!lua_isnoneornil(L, 4)) {
        luaL_checkany(L, 4);
        normals = (float *) lua_touserdata(L, 4);
    }
    float *uvs = NULL;
    if (!lua_isnoneornil(L, 5)) {
        luaL_checkany(L, 5);
        uvs = (float *) lua_touserdata(L, 5);
    }
    ngl_model *model = ngl_model_new(component_count, point_count, positions, normals, uvs);
    l_to_table(L, "ngl_model", model);
    return  1;
}

static int l_ngl_model_new_grid_points(lua_State *L) {
    int row_count = luaL_checkint(L, 1);
    int column_count = luaL_checkint(L, 2);
    float row_height = luaL_checknumber(L, 3);
    float column_width = luaL_checknumber(L, 4);
    ngl_model *model = ngl_model_new_grid_points(row_count, column_count, row_height, column_width);
    l_to_table(L, "ngl_model", model);
    return 1;
}

static int l_ngl_model_new_grid_triangles(lua_State *L) {
    int row_count = luaL_checkint(L, 1);
    int column_count = luaL_checkint(L, 2);
    float row_height = luaL_checknumber(L, 3);
    float column_width = luaL_checknumber(L, 4);
    ngl_model *model = ngl_model_new_grid_triangles(row_count, column_count, row_height, column_width);
    l_to_table(L, "ngl_model", model);
    return 1;
}

static int l_ngl_model_new_with_height_map(lua_State *L) {
    int row_count = luaL_checkint(L, 1);
    int column_count = luaL_checkint(L, 2);
    float row_height = luaL_checknumber(L, 3);
    float column_width = luaL_checknumber(L, 4);
    float height_multiplier = luaL_checknumber(L, 5);
    int buffer_stride = luaL_checkint(L, 6);
    int buffer_offset = luaL_checkint(L, 7);
    luaL_checkany(L, 8);
    float *positions = (float *) lua_touserdata(L, 8);
    ngl_model *model = ngl_model_new_with_height_map(row_count, column_count, row_height, column_width, height_multiplier, buffer_stride, buffer_offset, positions);
    l_to_table(L, "ngl_model", model);
    return 1;
}

static int l_ngl_model_load_obj(lua_State *L) {
    const char *fname = lua_tostring(L, 1);
    ngl_model *model = ngl_model_load_obj(fname);
    l_to_table(L, "ngl_model", model);
    return 1;
}

static int l_ngl_model_translate(lua_State *L) {
    ngl_model* model = l_to_ngl_model(L, 1);
    float tx = luaL_checknumber(L, 2);
    float ty = luaL_checknumber(L, 3);
    float tz = luaL_checknumber(L, 4);
    ngl_model_translate(model, tx, ty, tz);
    return 0;
}

static int l_ngl_model_free(lua_State *L) {
    ngl_model *model = l_to_ngl_model(L, 1);
    ngl_model_free(model);
    return 0;
}

static int l_ngl_draw_model(lua_State *L) {
    ngl_camera* camera = l_to_ngl_camera(L, 1);
    ngl_model* model = l_to_ngl_model(L, 2);
    ngl_shader *shader = l_to_ngl_shader(L, 3);
    ngl_draw_model(camera, model, shader);
    return 0;
}

// Lua NRF wrappers /////////////////////////////////////////////////////////

static int l_nrf_device_new(lua_State *L) {
    double freq_mhz = luaL_checknumber(L, 1);
    const char *file_name = lua_tostring(L, 2);
    double interpolate_step;
    if (!lua_isnoneornil(L, 3)) {
        interpolate_step = luaL_checknumber(L, 3);
    } else {
        interpolate_step = 1;
    }
    nrf_device *device = nrf_device_new(freq_mhz, file_name, interpolate_step);
    l_to_table(L, "nrf_device", device);

    lua_pushliteral(L, "samples");
    lua_pushlightuserdata(L, device->samples);
    lua_settable(L, -3);

    lua_pushliteral(L, "iq");
    lua_pushlightuserdata(L, device->iq);
    lua_settable(L, -3);

    lua_pushliteral(L, "fft");
    lua_pushlightuserdata(L, device->fft);
    lua_settable(L, -3);

    return 1;
}

static int l_nrf_device_free(lua_State *L) {
    nrf_device* device = l_to_nrf_device(L, 1);
    nrf_device_free(device);
    return 0;
}

static int l_nrf_device_set_frequency(lua_State *L) {
    nrf_device* device = l_to_nrf_device(L, 1);
    double freq_mhz = luaL_checknumber(L, 2);
    freq_mhz = nrf_device_set_frequency(device, freq_mhz);
    lua_pushnumber(L, freq_mhz);
    return 1;
}

// Main /////////////////////////////////////////////////////////////////////

int use_vr = 0;
nvr_device *device = NULL;

void usage() {
    printf("Usage: frequensea [--vr] FILE.lua\n");
    printf("Options:\n");
    printf("    --vr    Render to Oculus VR\n");
}

int str_ends_with(const char *s, const char *suffix) {
    if (!s || !suffix) {
        return 0;
    }
    size_t s_length = strlen(s);
    size_t suffix_length = strlen(suffix);
    if (suffix_length >  s_length) {
        return 0;
    }
    return strncmp(s + s_length - suffix_length, suffix, suffix_length) == 0;
}

static void draw(lua_State *L) {
    int error = l_call_function(L, "draw");
    if (error) {
        exit(EXIT_FAILURE);
    }
}

typedef struct {
    pthread_t thread;
    int width;
    int height;
    uint8_t *buffer;
    char *fname;
} screenshot_info;

static void _take_screenshot(screenshot_info *info) {
    const char *real_fname;
    if (info->fname == NULL) {
        time_t t;
        time(&t);
        struct tm* tm_info = localtime(&t);
        char time_fname[100];
        strftime(time_fname, 100, "screenshot-%Y-%m-%d_%H.%M.%S.png", tm_info);
        real_fname = time_fname;
    } else {
        real_fname = info->fname;
    }
    nim_png_write(real_fname, info->width, info->height, NIM_RGB, info->buffer);
    free(info->fname);
    free(info->buffer);
    free(info);
}

static void take_screenshot(nwm_window *window, const char *fname) {
    screenshot_info *info = calloc(1, sizeof(screenshot_info));
    if (fname != NULL) {
        info->fname = calloc(strlen(fname), sizeof(char));
        strcpy(info->fname, fname);
    }

    // Capture the OpenGL framebuffer. Do this in the current thread.
    glfwGetFramebufferSize(window, &info->width, &info->height);
    int buffer_channels = 3;
    int buffer_size = info->width * info->height * buffer_channels;
    info->buffer = calloc(buffer_size, sizeof(uint8_t));
    glReadPixels(0, 0, info->width, info->height, GL_RGB, GL_UNSIGNED_BYTE, info->buffer);

    // Create a new thread to save the screenshot.
    pthread_create(&info->thread, NULL, (void *(*)(void *))_take_screenshot, info);
}

static void draw_eye(nvr_device *device, nvr_eye *eye, lua_State *L) {
    ngl_camera *camera = nvr_device_eye_to_camera(device, eye);
    l_to_table(L, "ngl_camera", camera);
    lua_setglobal(L, "camera");
    draw(L);
}

static void on_key(nwm_window* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        } else if (key == GLFW_KEY_EQUAL) {
            take_screenshot(window, NULL);
        }
        if (use_vr) {
            static ovrHSWDisplayState hswDisplayState;
            ovrHmd_GetHSWDisplayState(device->hmd, &hswDisplayState);
            if (hswDisplayState.Displayed) {
                ovrHmd_DismissHSWDisplay(device->hmd);
                return;
            }
        }
        lua_State *L = nwm_window_get_user_data(window);
        if (L) {
            lua_getglobal(L, "on_key");
            if (lua_isfunction(L, -1)) {
                lua_pushinteger(L, key);
                lua_pushinteger(L, mods);
                int error = lua_pcall(L, 2, 0, 0);
                if (error) {
                    fprintf(stderr, "Error calling on_key(): %s\n", lua_tostring(L, -1));
                    lua_pop(L, 1);
                }
            } else {
                lua_pop(L, 1);
            }
        }
    }
}

// Initializes Lua
static lua_State *l_init() {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    l_register_type(L, "ngl_camera", l_ngl_camera_free);
    l_register_type(L, "ngl_model", l_ngl_model_free);
    l_register_type(L, "ngl_shader", l_ngl_shader_free);
    l_register_type(L, "ngl_texture", l_ngl_texture_free);
    l_register_type(L, "nrf_device", l_nrf_device_free);

    l_register_function(L, "nwm_get_time", l_nwm_get_time);
    l_register_function(L, "ngl_clear", l_ngl_clear);
    l_register_function(L, "ngl_camera_new_look_at", l_ngl_camera_new_look_at);
    l_register_function(L, "ngl_shader_new", l_ngl_shader_new);
    l_register_function(L, "ngl_shader_new_from_file", l_ngl_shader_new_from_file);
    l_register_function(L, "ngl_texture_new", l_ngl_texture_new);
    l_register_function(L, "ngl_texture_update", l_ngl_texture_update);
    l_register_function(L, "ngl_model_new", l_ngl_model_new);
    l_register_function(L, "ngl_model_new_grid_points", l_ngl_model_new_grid_points);
    l_register_function(L, "ngl_model_new_grid_triangles", l_ngl_model_new_grid_triangles);
    l_register_function(L, "ngl_model_new_with_height_map", l_ngl_model_new_with_height_map);
    l_register_function(L, "ngl_model_load_obj", l_ngl_model_load_obj);
    l_register_function(L, "ngl_model_translate", l_ngl_model_translate);
    l_register_function(L, "ngl_draw_model", l_ngl_draw_model);
    l_register_function(L, "nrf_device_new", l_nrf_device_new);
    l_register_function(L, "nrf_device_free", l_nrf_device_free);
    l_register_function(L, "nrf_device_set_frequency", l_nrf_device_set_frequency);

    l_register_constant(L, "NRF_SAMPLES_SIZE", NRF_SAMPLES_SIZE);
    l_register_constant(L, "GL_RED", GL_RED);
    l_register_constant(L, "GL_RG", GL_RG);
    l_register_constant(L, "GL_RGB", GL_RGB);
    l_register_constant(L, "GL_RGBA", GL_RGBA);
    l_register_constant(L, "GL_POINTS", GL_POINTS);
    l_register_constant(L, "GL_LINE_STRIP", GL_LINE_STRIP);
    l_register_constant(L, "GL_LINE_LOOP", GL_LINE_LOOP);
    l_register_constant(L, "GL_LINES", GL_LINES);
    l_register_constant(L, "GL_TRIANGLE_STRIP", GL_TRIANGLE_STRIP);
    l_register_constant(L, "GL_TRIANGLE_FAN", GL_TRIANGLE_FAN);
    l_register_constant(L, "GL_TRIANGLES", GL_TRIANGLES);

    int error = luaL_loadfile(L, "../lua/_keys.lua") || lua_pcall(L, 0, 0, 0);
    if (error) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    return L;
}

int main(int argc, char **argv) {
    int frame = 1;
    int capture = 0;
    char *fname = NULL;
    int window_width = 800;
    int window_height = 600;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage();
            exit(0);
        } else if (strcmp(argv[i], "--vr") == 0) {
            use_vr = 1;
        } else if (strcmp(argv[i], "--capture") == 0) {
            capture = 1;
        } else if (str_ends_with(argv[i], ".lua")) {
            fname = argv[i];
        }
    }

    if (fname == NULL) {
        usage();
        exit(0);
    }

    int error;

    lua_State *L = l_init();

    long old_mtime = nfile_mtime(fname);
    long frames_to_check = 10;

    error = luaL_loadfile(L, fname) || lua_pcall(L, 0, 0, 0);
    if (error) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    nwm_init();
    nwm_window *window = NULL;
    if (use_vr) {
        device = nvr_device_init();
        window = nvr_device_window_init(device);
        nvr_device_init_eyes(device);
    } else {
        window = nwm_window_init(0, 0, window_width, window_height);
    }
    assert(window);
    nwm_window_set_user_data(window, L);
    nwm_window_set_key_callback(window, on_key);

    error = l_call_function(L, "setup");
    if (error) {
        exit(EXIT_FAILURE);
    }

    while (!nwm_window_should_close(window)) {
        frames_to_check--;
        if (frames_to_check <= 0) {
            long new_mtime = nfile_mtime(fname);
            if (old_mtime != new_mtime) {
                fprintf(stderr, "Reloading (%lu)\n", new_mtime);
                // Close the Lua context. This triggers garbage collection on all objects.
                lua_close(L);

                // Re-initialize Lua.
                L = l_init();
                nwm_window_set_user_data(window, L);

                // Load the file again
                error = luaL_loadfile(L, fname) || lua_pcall(L, 0, 0, 0);
                if (error) {
                    fprintf(stderr, "%s\n", lua_tostring(L, -1));
                    lua_pop(L, 1);
                }
                // Call setup
                error = l_call_function(L, "setup");
                if (error) {
                    exit(EXIT_FAILURE);
                }
                old_mtime = new_mtime;
            }
            frames_to_check = 10;
        }
        if (use_vr) {
            nvr_device_draw(device, (nvr_render_cb_fn)draw_eye, L);
        } else {
            draw(L);
            nwm_window_swap_buffers(window);
            if (capture) {
                char fname[200];
                snprintf(fname, 200, "out-%04d.png", frame);
                take_screenshot(window, fname);
            }
        }
        nwm_poll_events();
        frame++;
    }

    if (use_vr) {
        nvr_device_destroy(device);
    }
    nwm_window_destroy(window);
    nwm_terminate();

    lua_close(L);
}
