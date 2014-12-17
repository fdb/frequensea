nwm_init()
window = nwm_create_window(800, 600)
while not nwm_window_should_close(window) do
    nwm_frame_begin(window)
    ngl_clear(0.17, 0.24, 0.31, 1.0)
    nwm_frame_end(window)
end
nwm_destroy_window(window)
nwm_terminate()
