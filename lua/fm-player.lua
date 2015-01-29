-- Play FM radio. Has no visible interface.

function setup()
    freq = (100.9e6 + 50000) / 1e6
    device = nrf_device_new(freq, "../rfdata/rf-100.900-1.raw")
    player = nrf_player_new(device, NRF_DEMODULATE_WBFM)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1.0)
end

function on_key(key, mods)
    keys_frequency_handler(key, mods)
end
