#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ngl.h"
#include "nvr.h"

mat4 ovr_matrix_to_mat4(const ovrMatrix4f* om) {
    // OVR Matrix4 is stored in row-major order, but we use column-major order.
    mat4 m;
    m.m[0] = om->M[0][0];
    m.m[1] = om->M[1][0];
    m.m[2] = om->M[2][0];
    m.m[3] = om->M[3][0];
    m.m[4] = om->M[0][1];
    m.m[5] = om->M[1][1];
    m.m[6] = om->M[2][1];
    m.m[7] = om->M[3][1];
    m.m[8] = om->M[0][2];
    m.m[9] = om->M[1][2];
    m.m[10] = om->M[2][2];
    m.m[11] = om->M[3][2];
    m.m[12] = om->M[0][3];
    m.m[13] = om->M[1][3];
    m.m[14] = om->M[2][3];
    m.m[15] = om->M[3][3];
    return m;
}

vec3 ovr_vector3_to_vec3(const ovrVector3f* ov) {
    vec3 v;
    v.x = ov->x;
    v.y = ov->y;
    v.z = ov->z;
    return v;
}

quat ovr_quat_to_quat(const ovrQuatf* oq) {
    quat q;
    q.x = oq->x;
    q.y = oq->y;
    q.z = oq->z;
    q.w = oq->w;
    return q;
}

nvr_device *nvr_device_init() {
    ovr_Initialize();
    ovrHmd hmd = ovrHmd_Create(0);
    if (hmd == NULL) {
        hmd = ovrHmd_CreateDebug(ovrHmd_DK1);
        assert(hmd != NULL);
    }
    ovrHmd_ConfigureTracking(hmd, ovrTrackingCap_Orientation | ovrTrackingCap_Position, 0);
    ovrHmd_RecenterPose(hmd);
    nvr_device *device = calloc(1, sizeof(nvr_device));
    device->hmd = hmd;
    return device;
}

void nvr_device_destroy(nvr_device *device) {
    ovrHmd_Destroy(device->hmd);
    device->hmd = NULL;
}

nwm_window *nvr_device_window_init(nvr_device *device) {
    assert(device != NULL);
    ovrHmd hmd = device->hmd;
    glfwWindowHint(GLFW_DECORATED, 0);
    printf("Window %d %d %d %d\n", hmd->WindowsPos.x, hmd->WindowsPos.y, hmd->Resolution.w, hmd->Resolution.h);
    nwm_window* window = nwm_window_init(hmd->WindowsPos.x, hmd->WindowsPos.y, hmd->Resolution.w, hmd->Resolution.h);
    assert(window);
    void *window_ptr = NULL;
#if __APPLE__
    window_ptr = glfwGetCocoaWindow(window);
#endif
    ovrHmd_AttachToWindow(hmd, window_ptr, NULL, NULL);
    ovrHmd_SetEnabledCaps(hmd, ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);
    assert(!glfwGetWindowAttrib(window, GLFW_DECORATED));
    return window;
}

void nvr_device_init_eyes(nvr_device *device) {
    ovrHmd hmd = device->hmd;
    printf("Product: %s\n", hmd->ProductName);

    ovrFovPort eyeFovPorts[2];
    for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++) {
        ovrEyeType eye_type = eyeIndex == 0 ? ovrEye_Left : ovrEye_Right;
        nvr_eye *eye;
        if (eye_type == 0) {
            eye = &device->left_eye;
        } else {
            eye = &device->right_eye;
        }
        eye->type = eye_type;
        ovrTextureHeader *eyeTextureHeader = &eye->texture.Header;
        eyeFovPorts[eye_type] = hmd->DefaultEyeFov[eye_type];
        ovrSizei texture_size = ovrHmd_GetFovTextureSize(hmd, eye_type, hmd->DefaultEyeFov[eye_type], 1.0f);
        eye->width = texture_size.w;
        eye->height = texture_size.h;
        eyeTextureHeader->TextureSize = texture_size;
        eyeTextureHeader->RenderViewport.Size = eyeTextureHeader->TextureSize;
        eyeTextureHeader->RenderViewport.Pos.x = 0;
        eyeTextureHeader->RenderViewport.Pos.y = 0;
        eyeTextureHeader->API = ovrRenderAPI_OpenGL;
        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        NGL_CHECK_ERROR();

        GLuint color_texture;
        glGenTextures(1, &color_texture);
        glBindTexture(GL_TEXTURE_2D, color_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_size.w, texture_size.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture, 0);
        NGL_CHECK_ERROR();

        GLuint depth_texture;
        glGenTextures(1, &depth_texture);
        glBindTexture(GL_TEXTURE_2D, depth_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, texture_size.w, texture_size.h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0);
        NGL_CHECK_ERROR();

        GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        assert(status == GL_FRAMEBUFFER_COMPLETE);
        NGL_CHECK_ERROR();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        NGL_CHECK_ERROR();

        ovrGLTexture *gl_texture = (ovrGLTexture *) &eye->texture;
        gl_texture->OGL.TexId = color_texture;
        eye->fbo = fbo;
    }

    ovrGLConfigData cfg;
    memset(&cfg, 0, sizeof(ovrGLConfigData));
    cfg.Header.API = ovrRenderAPI_OpenGL;
    cfg.Header.RTSize = hmd->Resolution;
    cfg.Header.Multisample = 1;

    int distortionCaps =
    ovrDistortionCap_TimeWarp |
    ovrDistortionCap_Chromatic |
    ovrDistortionCap_Vignette;

    ovrEyeRenderDesc eyeRenderDescs[2];
    ovrHmd_ConfigureRendering(hmd, (ovrRenderAPIConfig *)&cfg, distortionCaps, eyeFovPorts, eyeRenderDescs);

    for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++) {
        ovrEyeType eye_type = eyeIndex == 0 ? ovrEye_Left : ovrEye_Right;
        nvr_eye *eye;
        if (eye_type == 0) {
            eye = &device->left_eye;
        } else {
            eye = &device->right_eye;
        }
        ovrMatrix4f projection = ovrMatrix4f_Projection(eyeFovPorts[eyeIndex], 0.01f, 10000.0f, 1);
        eye->projection = ovr_matrix_to_mat4(&projection);
        ovrVector3f viewAdjust = eyeRenderDescs[eye_type].ViewAdjust;
        eye->view_adjust = ovr_vector3_to_vec3(&viewAdjust);
    }
}

void nvr_device_draw(nvr_device *device, nvr_render_cb_fn callback, void* ctx) {
    ovrHmd hmd = device->hmd;
    static int frameIndex = 0;
    static ovrPosef eyePoses[2];
    ovrHmd_BeginFrame(hmd, frameIndex++);
    glEnable(GL_DEPTH_TEST);
    for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++) {
        ovrEyeType eye_type = hmd->EyeRenderOrder[eyeIndex];
        nvr_eye *eye;
        if (eye_type == 0) {
            eye = &device->left_eye;
        } else {
            eye = &device->right_eye;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, eye->fbo);
        glViewport(0, 0, eye->width, eye->height);
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Apply the per-eye offset & the head pose
        eyePoses[eye_type] = ovrHmd_GetEyePose(hmd, eye_type);
        callback(device, eye, ctx);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    ovrTexture textures[2];
    textures[0] = device->left_eye.texture;
    textures[1] = device->right_eye.texture;
    ovrHmd_EndFrame(hmd, eyePoses, textures);
}

ngl_camera *nvr_device_eye_to_camera(nvr_device *device, nvr_eye *eye) {
    ovrHmd hmd = device->hmd;
    ovrPosef eyePose = ovrHmd_GetEyePose(hmd, eye->type);
    quat q = ovr_quat_to_quat(&eyePose.Orientation);
    mat4 orientation = quat_to_mat4(&q);
    mat4 position = mat4_init_identity();
    mat4 eye_pose = mat4_mul(&orientation, &position);
    mat4 inv_eye_pose = mat4_inverse(&eye_pose);
    ngl_camera *camera = calloc(1, sizeof(ngl_camera));
    camera->view = inv_eye_pose;
    camera->projection = eye->projection;
    return camera;
}

