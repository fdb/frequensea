-- Receive OSC events

function handle_message(path, args)
    print("OSC path: " .. path .. " len: " .. #args .. " 1: " .. args[1])
end

function setup()
    server = nosc_server_new(2222, handle_message)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1)
end
