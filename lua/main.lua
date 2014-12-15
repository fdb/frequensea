nwm_init()
window = nwm_create_window(800, 600)
while not nwm_window_should_close(window) do
    nwm_poll_events()
    nwm_swap_buffers(window)
end
nwm_destroy_window(window)
nwm_terminate()
