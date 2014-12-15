#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "ngl.h"
#include "nwm.h"

// Lua utility functions ////////////////////////////////////////////////////

static GLFWwindow* l_to_window(lua_State *L, int i) {
    lua_pushliteral(L,"__ptr__");
    lua_gettable(L, i);
    GLFWwindow * window=(GLFWwindow *)lua_touserdata(L,-1);
    lua_pop(L, 1);
    return window;
}

static void l_register_function(lua_State *L, void *func, const char *name) {
    lua_pushcfunction(L, func);
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
    lua_newtable(L);
    lua_pushliteral(L,"__ptr__");
    lua_pushlightuserdata(L, window);
    lua_settable(L, -3);
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

static int l_nwm_poll_events(lua_State *L) {
    nwm_poll_events();
    return 0;
}

static int l_nwm_swap_buffers(lua_State *L) {
    GLFWwindow *window = l_to_window(L, 1);
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

static int l_ngl_load_shader(lua_State *L) {
    const char *vertex_fname = lua_tostring(L, 1);
    const char *fragment_fname = lua_tostring(L, 2);

    GLuint shader = ngl_load_shader(vertex_fname, fragment_fname);
    lua_pushinteger(L, shader);
    return 1;
}

static int l_ngl_load_obj(lua_State *L) {
    const char *fname = lua_tostring(L, 1);
    ngl_model *model = ngl_load_obj(fname);
    lua_newtable(L);
    lua_pushliteral(L,"__ptr__");
    lua_pushlightuserdata(L, model);
    lua_settable(L, -3);
    return 1;
}

// Main /////////////////////////////////////////////////////////////////////

void usage() {
    printf("Usage: frequensea FILE.lua\n");
}

int main(int argc, char **argv) {
    char buff[256];
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

    l_register_function(L, l_nwm_init, "nwm_init");
    l_register_function(L, l_nwm_create_window, "nwm_create_window");
    l_register_function(L, l_nwm_destroy_window, "nwm_destroy_window");
    l_register_function(L, l_nwm_window_should_close, "nwm_window_should_close");
    l_register_function(L, l_nwm_poll_events, "nwm_poll_events");
    l_register_function(L, l_nwm_swap_buffers, "nwm_swap_buffers");
    l_register_function(L, l_nwm_terminate, "nwm_terminate");
    l_register_function(L, l_ngl_clear, "ngl_clear");
    l_register_function(L, l_ngl_load_shader, "ngl_load_shader");
    l_register_function(L, l_ngl_load_obj, "ngl_load_obj");

    error = luaL_loadfile(L, fname) || lua_pcall(L, 0, 0, 0);
    if (error) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    lua_close(L);
}
