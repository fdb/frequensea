#ifndef NVR_H
#define NVR_H

#ifdef __APPLE__
    #define OVR_OS_MAC
    #define GLFW_EXPOSE_NATIVE_COCOA
    #define GLFW_EXPOSE_NATIVE_NSGL
#endif

#include "OVR_CAPI.h"
#include "OVR_CAPI_GL.h"

#include "vec.h"
#include "nwm.h"

typedef struct {
    ovrEyeType type;
    int width;
    int height;
    mat4 projection;
    vec3 view_adjust;
    GLuint fbo;
    ovrTexture texture;
} nvr_eye;

typedef struct {
    ovrHmd hmd;
    nvr_eye left_eye;
    nvr_eye right_eye;
} nvr_device;

typedef void (*nvr_render_cb_fn)(nvr_device *device, nvr_eye *eye, void *);

nvr_device *nvr_device_init();
void nvr_device_destroy(nvr_device *device);
nwm_window *nvr_device_window_init(nvr_device *device);
void nvr_device_init_eyes(nvr_device *device);
void nvr_device_draw(nvr_device *device, nvr_render_cb_fn callback, void* ctx);
ngl_camera *nvr_device_eye_to_camera(nvr_device *device, nvr_eye *eye);

#endif // NVR_H
