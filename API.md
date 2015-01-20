## NWM -- NDBX Window Manager

Here's a typical example of a script:

    function setup()
        -- Load the model, shaders, camera, ...
    end

    function draw()
        ngl_clear(0.2, 0.2, 0.2, 1)
        -- Draw your scene
    end

## nwm_get_time()
Get the current time, in seconds. This is a floating-point number, so the
fractional part contains greater precision.

## NGL -- NDBX OpenGL API
The API that loads models, shaders, initializes the camera and can draw things on screen.

## ngl_clear(red, green, blue, alpha)
Clear the screen. The alpha component doesn't do anything at the moment, I don't know why.

## ngl_camera_new_look_at(x, y, z)
Create a camera object that is positioned at the given x/y/z values and looks at
the origin (0,0,0) of the scene.

This function returns a camera object that can be used with `ngl_draw_model`.

## ngl_shader_new(draw_mode, vertex_shader, fragment_shader)
Create a new shader using the given vertex shader and fragment shader source.
Note that you can use Lua's multi-line strings:

    VERTEX_SHADER = [[
        #version 400
        <Rest of the shader code goes here>
    ]]

The first argument is a OpenGL draw mode. For a scene loaded using `ngl_model_load_obj`
this should probably be GL_TRIANGLES.

Other modes are `GL_POINTS`, `GL_LINE_STRIP`, `GL_LINE_LOOP`, `GL_LINES`, `GL_TRIANGLE_STRIP`, `GL_TRIANGLE_FAN`, `GL_TRIANGLES`. See [glDrawArrays](https://www.opengl.org/sdk/docs/man3/xhtml/glDrawArrays.xml) for more info on drawing modes.

## ngl_shader_new_from_file(draw_mode, vertex_file, fragment_file)
Create a new shader by loading the vertex and fragment file. The first argument is a OpenGL draw mode;
for a scene loaded using `ngl_model_load_obj` this should probably be GL_TRIANGLES.

Other modes are `GL_POINTS`, `GL_LINE_STRIP`, `GL_LINE_LOOP`, `GL_LINES`, `GL_TRIANGLE_STRIP`, `GL_TRIANGLE_FAN`, `GL_TRIANGLES`. See [glDrawArrays](https://www.opengl.org/sdk/docs/man3/xhtml/glDrawArrays.xml) for more info on drawing modes.

## ngl_model_new(component_count, point_count, positions, normals)
Create a new model by initializing it with a list of points. The number of components should be 2 for 2D X/Y points, and 3 for 3D X/Y/Z points. The number of points is the number of points passed in. The positions is an array coming from other functions, like NRF's `device.samples` (Lua lists don't work yet). The positions are packed: so a 3D array would be passed in as XYZXYZXYZ... . The total length of the array is component_count * point_count.

The `positions` and `normals` arguments are both optional.

This function returns a model object that can be used in `ngl_draw_model`.

## ngl_texture_new(shader, uniform_name)
Create an empty texture object. The name refers to the texture uniform name in the shader. Returns a texture object.

## ngl_texture_update(texture, format, width, height, data)
Set the texture data. `texture` is the texture object returned by `ngl_texture_new.` `format` is one of `GL_RED`, `GL_RG`, `GL_RGB` or `GL_RGBA`. `width` and `height` refer to texture size. Ideally, these are powers of two (e.g. 256, 512, 1024). `data` is a buffer containing floating-point values. We assume data has `n_components * width * height` floats. If the data is too small, the program will crash.

## ngl_model_new_grid_points(row_count, column_count, row_height, column_width)
Initialize a two-dimensional grids with one point per grid position. The grid will have row_count * column_count points.
`row_height` and `column_width` specify the distance between each row and column. This model won't have any normals.

## ngl_model_new_grid_triangles(row_count, column_count, row_height, column_width);
Initialize a grid that can be used to draw triangles. Point positions are in the X and Z components. The grid will have (row_count-1) * (column_count -1) * 6 points (two triangles per grid "square"). `row_height` and `column_width` specify the distance between each row and column. This model has normals.

## ngl_model_load_obj(file)
Load an OBJ file from disk. The OBJ file should only use triangles, and should have normals exported.

This function returns a model object that can be used in `ngl_draw_model`.

## ngl_model_translate(model, tx, ty, tz)
Translate the model. The model is transformed in-place; no data is returned.

## ngl_draw_model(camera, model, shader)
Draw the model with the given shader. Use the same camera object if you want consistent views of the scene.

## NRF -- NDBX Radio Frequency
Functions for reading data from a HackRF device.

Here's a basic example:

    device = nrf_device_new(204.0)
    while true do
        -- Do something with device.samples
    end
    nrf_device_free(device)

## nrf_device_new(freq_mhz)
Tune the HackRF to the given frequency (in MHz) and start receiving data. The function returns a device object. The device object has a member, `samples`, that contains a list of NRF_SAMPLES_SIZE three-component floating-point values, containing (i, q, t) where t is a value between 0.0 (beginning of the sample data) to 1.0 (end of the sample data).

## nrf_device_free(device)
Stop receiving data.

### nrf_device_set_frequency(device, freq_mhz)
Change the frequency device to the given frequency (in MHz). The `device` is a device object as returned by `nrf_device_new`.

## NRF_SAMPLES_SIZE
A constant with the number of samples the HackRF device returns.
