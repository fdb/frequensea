-- Visualize IQ data from the HackRF
nwm_init()
nrf_start(100.9)
window = nwm_create_window(800, 600)

while not nwm_window_should_close(window) do
    ngl_clear(0.2, 0.2, 0.2, 1)

    nwm_poll_events()
    nwm_swap_buffers(window)
end

nrf_stop()
nwm_destroy_window(window)
nwm_terminate()
