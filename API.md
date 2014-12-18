## NWM -- NDBX Window Manager

Here's a typical example of a script:

    nwm_init()
    window = nwm_create_window(800, 600)

    -- Setup code goes here

    while not nwm_window_should_close(window) do
        nwm_frame_begin(window)

        --- Main drawing code here

        nwm_frame_end(window)
    end
    nwm_destroy_window(window)
    nwm_terminate()


### nwm_init()
Initializes the window manager. Should be at the top of your script.

### nwm_create_window(width, height)
Create a new window with the given width and height. The window is positioned in the center of the screen by default.

This function returns a window object, which you should hold on to. It is needed to draw frames and to close itself.

### nwm_destroy_window(window)
Close the window.

### nwm_window_should_close(window)
Check if the user has performed an event that should close the window, like clicking the "close" button.
This function is used in the main loop:

    while not nwm_window_should_close(window) do
        -- Per-frame code
    end

### nwm_frame_begin(window)
Set up the frame. This should be called at the start of the main loop.

### nwm_frame_end(window)
End the frame. This code checks for events and swaps drawing buffers. This should be called at the end of the main loop.

### nwm_terminate()
Clean up left over resources and end the application. This should be the last line in your script.

## NGL -- NDBX OpenGL API
The API that loads models, shaders, initializes the camera and can draw things on screen.

## ngl_clear(red, green, blue, alpha)
Clear the screen. The alpha component doesn't do anything at the moment, I don't know why.

## ngl_camera_init_look_at(x, y, z)
Create a camera object that is positioned at the given x/y/z values and looks at
the origin (0,0,0) of the scene.

This function returns a camera object that can be used with `ngl_draw_model`.

## ngl_load_shader(draw_mode, vertex_file, fragment_file)
Create a new shader by loading the vertex and fragment file. The first argument is a OpenGL draw mode;
for a scene loaded using `ngl_load_obj` this should probably be GL_TRIANGLES.

Other modes are `GL_POINTS`, `GL_LINE_STRIP`, `GL_LINE_LOOP`, `GL_LINES`, `GL_TRIANGLE_STRIP`, `GL_TRIANGLE_FAN`, `GL_TRIANGLES` and `GL_PATCHES`. See [glDrawArrays](https://www.opengl.org/sdk/docs/man3/xhtml/glDrawArrays.xml) for more info on drawing modes.

## ngl_model_init_positions(component_count, point_count, positions, normals)
Create a new model by initializing it with a list of points. The number of components should be 2 for 2D X/Y points, and 3 for 3D X/Y/Z points. The number of points is the number of points passed in. The positions is an array coming from other functions, like NRF's `device.samples` (Lua lists don't work yet). The positions are packed: so a 3D array would be passed in as XYZXYZXYZ... . The total length of the array is component_count * point_count.

The `positions` and `normals` arguments are both optional.

This function returns a model object that can be used in `ngl_draw_model`.

## ngl_load_obj(file)
Load an OBJ file from disk. The OBJ file should only use triangles, and should have normals exported.

This function returns a model object that can be used in `ngl_draw_model`.

## ngl_draw_model(camera, model, shader)
Draw the model with the given shader. Use the same camera object if you want consistent views of the scene.

## NRF -- NDBX Radio Frequency
Functions for reading data from a HackRF device.

Here's a basic example:

    device = nrf_start(204.0)
    while true do
        -- Do something with device.samples
    end
    nrf_stop(device)

## nrf_start(freq_mhz)
Tune the HackRF to the given frequency (in MHz) and start receiving data. The function returns a device object. The device object has a member, `samples`, that contains a list of NRF_SAMPLES_SIZE three-component floating-point values, containing (i, q, t) where t is a value between 0.0 (beginning of the sample data) to 1.0 (end of the sample data).

## nrf_stop(device)
Stop receiving data.

## NRF_SAMPLES_SIZE
A constant with the number of samples the HackRF device returns.
