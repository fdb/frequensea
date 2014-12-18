#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "ngl.h"
#include "nrf.h"
#include "nwm.h"
#include "vec.h"

// Lua utility functions ////////////////////////////////////////////////////


static void l_to_table(lua_State *L, char *type, void *obj) {
    lua_newtable(L);
    lua_pushliteral(L, "__type__");
    lua_pushstring(L, type);
    lua_settable(L, -3);
    lua_pushliteral(L, "__ptr__");
    lua_pushlightuserdata(L, obj);
    lua_settable(L, -3);
}

static void* l_from_table(lua_State *L, char *type, int index) {
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
        exit(-1);
    }
}

static GLFWwindow* l_to_window(lua_State *L, int index) {
    return (GLFWwindow*) l_from_table(L, "nwm_window", index);
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

// Lua NWM wrappers /////////////////////////////////////////////////////////

static int l_nwm_init(lua_State *L) {
    nwm_init();
    return 0;
}

static int l_nwm_create_window(lua_State *L) {
    int width = luaL_checkint(L, 1);
    int height = luaL_checkint(L, 2);

    GLFWwindow *window = nwm_create_window(width, height);
    l_to_table(L, "nwm_window", window);
    return 1;
}

static int l_nwm_destroy_window(lua_State *L) {
    GLFWwindow *window = l_to_window(L, 1);
    nwm_destroy_window(window);
    return 0;
}

static int l_nwm_window_should_close(lua_State *L) {
    int b = nwm_window_should_close(l_to_window(L, 1));
    lua_pushboolean(L, b);
    return 1;
}

static int l_nwm_frame_begin(lua_State *L) {
    return 0;
}

static int l_nwm_frame_end(lua_State *L) {
    GLFWwindow *window = l_to_window(L, 1);
    nwm_poll_events();
    nwm_swap_buffers(window);
    return 0;
}

static int l_nwm_terminate(lua_State *L) {
    nwm_terminate();
    return 0;
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

static int l_ngl_camera_init_look_at(lua_State *L) {
    float camera_x = luaL_checknumber(L, 1);
    float camera_y = luaL_checknumber(L, 2);
    float camera_z = luaL_checknumber(L, 3);
    ngl_camera *camera = ngl_camera_init_look_at(camera_x, camera_y, camera_z);
    l_to_table(L, "ngl_camera", camera);
    return 1;
}

static int l_ngl_load_shader(lua_State *L) {
    GLenum draw_mode = luaL_checkint(L, 1);
    const char *vertex_fname = lua_tostring(L, 2);
    const char *fragment_fname = lua_tostring(L, 3);

    ngl_shader *shader = ngl_load_shader(draw_mode, vertex_fname, fragment_fname);
    l_to_table(L, "ngl_shader", shader);
    return 1;
}

static int l_ngl_model_init_positions(lua_State *L) {
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
    ngl_model *model = ngl_model_init_positions(component_count, point_count, positions, normals);
    l_to_table(L, "ngl_model", model);
    return  1;
}

static int l_ngl_model_init_grid(lua_State *L) {
    int row_count = luaL_checkint(L, 1);
    int column_count = luaL_checkint(L, 2);
    float row_height = luaL_checknumber(L, 3);
    float column_width = luaL_checknumber(L, 4);
    ngl_model *model = ngl_model_init_grid(row_count, column_count, row_height, column_width);
    l_to_table(L, "ngl_model", model);
    return 1;
}

static int l_ngl_load_obj(lua_State *L) {
    const char *fname = lua_tostring(L, 1);
    ngl_model *model = ngl_load_obj(fname);
    l_to_table(L, "ngl_model", model);
    return 1;
}

static int l_ngl_draw_model(lua_State *L) {
    ngl_camera* camera = l_to_ngl_camera(L, 1);
    ngl_model* model = l_to_ngl_model(L, 2);
    ngl_shader *shader = l_to_ngl_shader(L, 3);
    ngl_draw_model(camera, model, shader);
    return 0;
}

// Lua NRF wrappers /////////////////////////////////////////////////////////

static int l_nrf_start(lua_State *L) {
    double freq_mhz = luaL_checknumber(L, 1);
    nrf_device *device = nrf_start(freq_mhz);
    l_to_table(L, "nrf_device", device);

    lua_pushliteral(L, "samples");
    lua_pushlightuserdata(L, device->samples);
    lua_settable(L, -3);

    return 1;
}

static int l_nrf_stop(lua_State *L) {
    nrf_device* device = l_to_nrf_device(L, 1);
    nrf_stop(device);
    return 0;
}

// Main /////////////////////////////////////////////////////////////////////

void usage() {
    printf("Usage: frequensea FILE.lua\n");
}

int main(int argc, char **argv) {
    char *fname;

    if (argc != 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        usage();
        exit(0);
    } else {
        fname = argv[1];
    }

    int error;
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    l_register_function(L, "nwm_init", l_nwm_init);
    l_register_function(L, "nwm_create_window", l_nwm_create_window);
    l_register_function(L, "nwm_destroy_window", l_nwm_destroy_window);
    l_register_function(L, "nwm_window_should_close", l_nwm_window_should_close);
    l_register_function(L, "nwm_frame_begin", l_nwm_frame_begin);
    l_register_function(L, "nwm_frame_end", l_nwm_frame_end);
    l_register_function(L, "nwm_terminate", l_nwm_terminate);
    l_register_function(L, "ngl_clear", l_ngl_clear);
    l_register_function(L, "ngl_camera_init_look_at", l_ngl_camera_init_look_at);
    l_register_function(L, "ngl_load_shader", l_ngl_load_shader);
    l_register_function(L, "ngl_model_init_positions", l_ngl_model_init_positions);
    l_register_function(L, "ngl_model_init_grid", l_ngl_model_init_grid);
    l_register_function(L, "ngl_load_obj", l_ngl_load_obj);
    l_register_function(L, "ngl_draw_model", l_ngl_draw_model);
    l_register_function(L, "nrf_start", l_nrf_start);
    l_register_function(L, "nrf_stop", l_nrf_stop);

    l_register_constant(L, "NRF_SAMPLES_SIZE", NRF_SAMPLES_SIZE);
    l_register_constant(L, "GL_POINTS", GL_POINTS);
    l_register_constant(L, "GL_LINE_STRIP", GL_LINE_STRIP);
    l_register_constant(L, "GL_LINE_LOOP", GL_LINE_LOOP);
    l_register_constant(L, "GL_LINES", GL_LINES);
    l_register_constant(L, "GL_TRIANGLE_STRIP", GL_TRIANGLE_STRIP);
    l_register_constant(L, "GL_TRIANGLE_FAN", GL_TRIANGLE_FAN);
    l_register_constant(L, "GL_TRIANGLES", GL_TRIANGLES);
    l_register_constant(L, "GL_PATCHES", GL_PATCHES);

    error = luaL_loadfile(L, fname) || lua_pcall(L, 0, 0, 0);
    if (error) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    lua_close(L);
}
