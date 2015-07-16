## Introduction

Here's a typical example of a script:

    function setup()
        -- Load the model, shaders, camera, ...
    end

    function draw()
        ngl_clear(0.2, 0.2, 0.2, 1)
        -- Draw your scene
    end

The Frequensea API uses a consistent naming scheme. Each call starts with the name of the module (nut, nwm, ngl, nosc or nrf), then the name of the object, then the action on that object. To illustrate, here's how you create a camera object:

    camera = ngl_camera_new()

To call a method on that object, use the object as the first argument:

    ngl_camera_rotate_x(camera, 45)

## NWM -- Window Manager

Currently you can't create, move or resize windows in Lua. You can call the "frequensea" binary with the `--width` and `--height` flags to change the size, e.g.:

    ./frequensea --width 1920 --height 1080 ../lua/static.lua

## nwm_get_time()
Get the current time, in seconds. This is a floating-point number, so the
fractional part contains greater precision.

## NGL -- OpenGL API

The API that loads models, shaders, initializes the camera and can draw things on screen.

### ngl_clear(red, green, blue, alpha)

Clear the screen and the depth buffer. The alpha component doesn't do anything at the moment, I don't know why.

### ngl_clear_depth()

Only clear the depth buffer; the screen is not cleared.

### ngl_camera

The `ngl_camera` object contains the current view and projection transforms. The projection is currently fixed: only the view transform (the position of the camera) can be modified.

### ngl_camera_new()

Create a new camera positioned at (0,0,0). Use `ngl_camera_translate` to move the camera. Returns a `ngl_camera` object that can be used with `ngl_draw_model`.

### ngl_camera_new_look_at(x, y, z)

Create a camera object that is positioned at the given x/y/z values and looks at
the origin (0,0,0) of the scene. Returns a camera object that can be used with `ngl_draw_model`.

### ngl_camera_translate(camera, x, y, z)

Translate (or move) the camera by the given x, y, z values. This changes the camera in-place, no new camera object is created.

### ngl_camera_rotate_x(camera, degrees) / ngl_camera_rotate_y(camera, degrees) / ngl_camera_rotate_z(camera, degrees)

Rotate the camera over the given axis by degrees. This changes the camera in-place, no new camera object is created.

### ngl_shader

The `ngl_shader` object contains the vertex and fragment shaders necessary for drawing objects on screen. It also contains the draw mode: an OpenGL constant that specifies whether you want to draw lines (`GL_LINES`, `GL_LINE_STRIP`, `GL_LINE_LOOP`) or triangles (`GL_TRIANGLES`, `GL_TRIANGLE_STRIP`, `GL_TRIANGLE_FAN`).

### ngl_shader_new(draw_mode, vertex_shader, fragment_shader)

Create a new shader using the given vertex shader and fragment shader source.
Note that you can use Lua's multi-line strings:

    VERTEX_SHADER = [[
        #version 400
        <Rest of the shader code goes here>
    ]]

The first argument is a OpenGL draw mode. For a scene loaded using `ngl_model_load_obj`
this should probably be GL_TRIANGLES.

Other modes are `GL_POINTS`, `GL_LINE_STRIP`, `GL_LINE_LOOP`, `GL_LINES`, `GL_TRIANGLE_STRIP`, `GL_TRIANGLE_FAN`, `GL_TRIANGLES`. See [glDrawArrays](https://www.opengl.org/sdk/docs/man3/xhtml/glDrawArrays.xml) for more info on drawing modes.

### ngl_shader_new_from_file(draw_mode, vertex_file, fragment_file)

Create a new shader by loading the vertex and fragment file. The first argument is a OpenGL draw mode;
for a scene loaded using `ngl_model_load_obj` this should probably be GL_TRIANGLES.

Other modes are `GL_POINTS`, `GL_LINE_STRIP`, `GL_LINE_LOOP`, `GL_LINES`, `GL_TRIANGLE_STRIP`, `GL_TRIANGLE_FAN`, `GL_TRIANGLES`. See [glDrawArrays](https://www.opengl.org/sdk/docs/man3/xhtml/glDrawArrays.xml) for more info on drawing modes.

### ngl_shader_uniform_set_float(shader, uniform_name, value)

Set the uniform in GLSL to the given value. This is useful to change a running shader on the fly.


### ngl_texture

Textures are images that are sent to the shader. These images can be loaded from a file (using `ngl_texture_new_from_file`) or updated on-the-fly, with data from a buffer (using `ngl_texture_update`).

### ngl_texture_new(shader, uniform_name)

Create an empty texture object. The name refers to the texture uniform name in the shader. Returns a `ngl_texture` object that can be used with `ngl_texture_update`.

### ngl_texture_new_from_file(file_name, shader, uniform_name)

Create a texture object from an image file. The name refers to the texture uniform name in the shader. Returns a `ngl_texture` object.

## ngl_texture_update(texture, buffer, width, height)

Set the texture data. `texture` is the texture object returned by `ngl_texture_new.` `buffer` is a `nut_buffer`, such as returned from `nrf_device_get_samples_buffer`. Since buffers are one-dimensional, `width` and `height` are needed to change this into a two-dimensional texture. Note that width * height needs to match the size of the buffer. If the sizes are incorrect, the program will crash.

### ngl_model_new_with_buffer(buffer)

Create a new model by initializing it with a list of points. The model will have as many channels as the buffer, that is 2D for 2 channels, 3D for 3 channels. The points will not have normals or texture coordinates.

This function returns a `ngl_model` object that can be used in `ngl_draw_model`.

### ngl_model_new_grid_points(row_count, column_count, row_height, column_width)

Create a new model that contains a two-dimensional grids with one point per grid position. The grid will have row_count * column_count points. `row_height` and `column_width` specify the distance between each row and column. This model won't have any normals.

### ngl_model_new_grid_triangles(row_count, column_count, row_height, column_width)

Initialize a grid that can be used to draw triangles. Point positions are in the X and Z components. The grid will have (row_count-1) * (column_count -1) * 6 points (two triangles per grid "square"). `row_height` and `column_width` specify the distance between each row and column. This model has normals.

## ngl_model_load_obj(file)
Load an OBJ file from disk. The OBJ file should only use triangles, and should have normals exported.

This function returns a model object that can be used in `ngl_draw_model`.

## ngl_model_translate(model, tx, ty, tz)
Translate the model. The model is transformed in-place; no data is returned.

## ngl_draw_model(camera, model, shader)
Draw the model with the given shader. Use the same camera object if you want consistent views of the scene.

## ngl_capture_model(camera, model, shader, file_name)
Save the model to an OBJ file called `file_name`. This will use the [OpenGL Transform Feedback](https://www.opengl.org/wiki/Transform_Feedback) feature to save the output of the vertex shader. Currently, only gl_Position is saved. This call is very slow, so best is to trigger it on a key press. See `save-model.lua` for an example.

## NRF -- NDBX Radio Frequency
Functions for reading data from a software defined radio (SDR) device.

Here's a basic example:

    device = nrf_device_new(204.0)
    while true do
        -- Do something with device.samples
    end
    nrf_device_free(device)

## nrf_device_new(freq_mhz, data_file)
Tune the SDR device to the given frequency (in MHz) and start receiving data. We can receive data from RTL-SDR, HackRF, or fall back to a data file. The function returns a device object. The device object has a member, `samples`, that contains a list of NRF_SAMPLES_LENGTH three-component floating-point values, containing (i, q, t) where t is a value between 0.0 (beginning of the sample data) to 1.0 (end of the sample data).

### nrf_device_set_frequency(device, freq_mhz)
Change the frequency device to the given frequency (in MHz). The `device` is a device object as returned by `nrf_device_new`.

### nrf_device_set_paused(device, paused)
If `paused` is 1, stop receiving new blocks; instead just keep working on the current block. This currently only works for dummy devices.

### nrf_device_step(device)
Advance one block. This is only works if the device is paused using `nrf_device_set_paused(device, true)`

### nrf_device_get_samples_buffer(device)
Get the raw samples buffer. This returns a buffer object that can be used with ngl_texture_update, e.g.:

    buffer = nrf_device_get_samples_buffer(device)
    ngl_texture_update(texture, buffer.width, buffer.height, buffer.channels, buffer.data)

### nrf_device_get_iq_buffer(device)
Get the IQ values plotted as points. This returns a buffer object that can be used with ngl_texture_update.

### nrf_device_get_iq_lines(device, int size_multiplier, float line_percentage)
Get the IQ values plotted as lines. This returns a buffer object that can be used with ngl_texture_update. `size_multiplier` specifies the multiplication factor with regards to the IQ data resolution (so 256 * size_multiplier). The line_percentage is a value between 0-1 that specifies how much of the sample buffer to draw.

The buffer will have one color channel, which maps to the red channel in the fragment shader.

Note that lines can overlap, and the buffer values can be higher than 1.0. It is a good idea to scale the colors down in the shader.

### nrf_device_get_fft_buffer(device)
Get the FFT data as a buffer. This returns a buffer object that can be used with ngl_texture_update. The size is controlled by fft_size (the buffer width) and fft_history_size (the buffer_height). Both can be set when initializing the device, for example:

    device = nrf_device_new_with_config({freq_mhz=100.0, fft_width=1024, fft_history_size=2048})

## NUT -- Utilities

### nut_buffer

A number of functions, such as nrf_device_get_samples_buffer, return a `nut_buffer`. There are two types of nut_buffers: one that contain unsigned bytes (`NUT_BUFFER_U8`) and one that contains double-precision floating point values (`NUT_BUFFER_F64`).

You can't create new buffers in Lua, but you can modify them using the following commands:

### nut_buffer_append(dst, src)

Append the `src` buffer to the `dst` buffer. Both buffers need to be of the same type. The destination buffer is enlarged to fit the contents of both buffers. The `src` buffer is not modified.

### nut_buffer_reduce(buffer, percentage)

Return a new buffer that's a given percentage of the original buffer's size. Percentage is a value between 0.0-1.0. The returned buffer will have the same type as the input buffer.

### nut_buffer_clip(buffer, offset, length)

Return a new buffer that's a subset of the original buffer. The returned buffer will have the same type as the input buffer.

### nut_buffer_convert(buffer, new_type)

Return a new buffer that's converted from the original type. `new_type` can be `NUT_BUFFER_U8` for unsigned bytes or `NUT_BUFFER_F64` for double-precision floating point values. Converting to the same type just creates a copy of the buffer.

### nut_buffer_save(buffer, file_name)

Save the contents of the buffer to the given file. The buffer data is saved "as-is", that is, no conversion is performed.
