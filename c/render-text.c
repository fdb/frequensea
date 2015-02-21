#include <math.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../externals/stb/stb_truetype.h"

#include "easypng.h"

const char* font_file = "../fonts/Roboto-Bold.ttf";

int imax(const int a, const int b) {
    return a > b ? a : b;
}

void measure_text(const stbtt_fontinfo *font, const char *text, int x, int y, int font_size, int *width, int *height) {
    float font_scale = stbtt_ScaleForPixelHeight(font, font_size);
    int ascent, descent;
    stbtt_GetFontVMetrics(font, &ascent, &descent, 0);
    int baseline = (int) (ascent * font_scale);
    int descent_scaled = (int) (descent * font_scale);
    printf("Baseline %d descent %d\n", baseline, descent_scaled);

    *width = x;
    // The glyph height is constant for the font.
    // Descent_scaled is negative.
    *height = y + (baseline - descent_scaled);

    int ch = 0;
    while (text[ch]) {
        int x0, y0, x1, y1;
        int advance, lsb;
        char c = text[ch];
        stbtt_GetCodepointBitmapBox(font, c, font_scale, font_scale, &x0, &y0, &x1, &y1);
        stbtt_GetCodepointHMetrics(font, c, &advance, &lsb);
        int glyph_width = advance * font_scale;
        *width += glyph_width;
        if (text[ch + 1]) {
            *width += font_scale * stbtt_GetCodepointKernAdvance(font, c, text[ch + 1]);
        }
        ch++;
    }
}

// FIXME compose using alpha lookup
void img_gray_copy(uint8_t *dst, uint8_t *src, uint32_t dst_x, uint32_t dst_y, uint32_t src_x, uint32_t src_y, uint32_t width, uint32_t height, uint32_t dst_stride, uint32_t src_stride) {
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            dst[(dst_y + i) * dst_stride + dst_x + j] = src[(src_y + i) * src_stride + src_x + j];
        }
    }
}

int main() {
    FILE *fp = fopen(font_file, "rb");
    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    uint8_t *font_buffer = calloc(size, 1);
    fread(font_buffer, size, 1, fp);
    fclose(fp);
    printf("Font file size %ld\n", size);

    stbtt_fontinfo font;
    printf("Offset: %d\n", stbtt_GetFontOffsetForIndex(font_buffer, 0));
    stbtt_InitFont(&font, font_buffer, stbtt_GetFontOffsetForIndex(font_buffer, 0));

    const char *text = "Hello, World!";
    int font_size = 300;
    float font_scale = stbtt_ScaleForPixelHeight(&font, font_size);
    int text_width, text_height;
    measure_text(&font, text, 0, 0, font_size, &text_width, &text_height);
    printf("Text W: %d H: %d\n", text_width, text_height);

    int ascent;
    stbtt_GetFontVMetrics(&font, &ascent, 0, 0);
    int baseline = (int) (ascent * font_scale);


    // Make full image
    uint8_t *image_buffer = calloc(text_width * text_height, 1);
    int ch = 0;
    int x = 0;
    while (text[ch]) {
        int w, h, dx, dy;
        int advance, lsb;
        char c = text[ch];
        stbtt_GetCodepointHMetrics(&font, c, &advance, &lsb);
        uint8_t *glyph_bitmap = stbtt_GetCodepointBitmap(&font, font_scale, font_scale, c, &w, &h, &dx, &dy);
        printf("Offset %d %d\n", dx, baseline + dy);
        img_gray_copy(image_buffer, glyph_bitmap, x + dx, baseline + dy, 0, 0, w, h, text_width, w);
        //x += w;
        x += (advance * font_scale);

        if (text[ch + 1]) {
            x += font_scale * stbtt_GetCodepointKernAdvance(&font, c, text[ch + 1]);
        }

        ch++;
    }

    //draw_text(&font, "Hi!", 0, 0)
    write_gray_png("_text.png", text_width, text_height, image_buffer);







    // float font_scale = stbtt_ScaleForPixelHeight(&font, font_size);
    // int ascent;
    // stbtt_GetFontVMetrics(&font, &ascent, 0, 0);
    // printf("Sclae %.2f Ascent %d\n", font_scale, ascent);
    // //int baseline = (int) (ascent * font_scale);
    // int x_pos = 2; // Pad a little bit.


    // const char *text = "&";
    // int ch = 0;
    // while (text[ch]) {
    //     int advance, lsb;
    //     //float x_shift = x_pos - (float) floor(x_pos);
    //     char c = text[ch];
    //     stbtt_GetCodepointHMetrics(&font, c, &advance, &lsb);
    //     //stbtt_GetCodepointBitmapBoxSubpixel(&font, c, font_scale, font_scale, x_shift, 0, &x0, &y0, &x1, &y1);
    //     int w, h;
    //     unsigned char *bitmap = stbtt_GetCodepointBitmap(&font, 0, font_scale, c, &w, &h, 0, 0);
    //     printf("Width %d Height %d\n", w, h);
    //     write_gray_png("_text.png", w, h, bitmap);

    //     //sbtt_MakeCodepointBitmap(&font, );
    //     x_pos += (advance * font_scale);
    //     if (text[ch + 1]) {
    //         x_pos += font_scale * stbtt_GetCodepointKernAdvance(&font, text[ch], text[ch + 1]);
    //     }
    //     ch++;
    // }


    return 0;
}
