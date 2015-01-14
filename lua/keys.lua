-- Show keys

function setup()
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1)
end

function on_key(key)
    print("Key " .. key)
    if key == KEY_SPACE then
        print("Space")
    elseif key == KEY_DOWN then
        print("Down")
    end
end

