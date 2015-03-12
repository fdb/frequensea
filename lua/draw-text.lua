-- Example showing how to draw text.

function setup()
    font = ngl_font_new("../fonts/Roboto-Bold.ttf", 72)
end

function draw()
    ngl_clear(0.0, 0.0, 0.0, 1)
    ngl_font_draw(font, "Hello, world!", 1, 1, 1.0)
end
