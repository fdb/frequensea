-- Play FM radio. Has no visible interface.

function setup()
    freq_offset = 50000
    freq = (100.9e6 + freq_offset) / 1e6
    device = nrf_device_new(freq, "../rfdata/rf-100.900-1.raw")
    player = nrf_player_new(device, NRF_DEMODULATE_WBFM, freq_offset)
    freq_font = ngl_font_new("../fonts/Roboto-Bold.ttf", 72)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_font_draw(freq_font, freq, 1, 0.7, 1.0)
end

function on_key(key, mods)
    keys_frequency_handler(key, mods)
    keys_frequency_offset_handler(key, mods)
end
