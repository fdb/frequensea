-- Play FM radio. Has no visible interface.

function setup()
    freq_offset = 50000
    freq = (1500e6 + freq_offset) / 1e6
    device = nrf_device_new(freq, "../rfdata/rf-100.900-1.raw")
    player = nrf_player_new(device, NRF_DEMODULATE_WBFM, freq_offset)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1.0)
end

function on_key(key, mods)
    keys_frequency_handler(key, mods)
    if key == KEY_LEFT_BRACKET then
        freq_offset = freq_offset + 50000
        print(freq + (freq_offset / 1e6))
        nrf_player_set_freq_offset(player, freq_offset)
    elseif key == KEY_RIGHT_BRACKET then
        freq_offset = freq_offset - 50000
        print(freq + (freq_offset / 1e6))
        nrf_player_set_freq_offset(player, freq_offset)
    end
end
