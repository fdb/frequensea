// Add markers

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../externals/stb/stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../externals/stb/stb_truetype.h"

#include "easypng.h"

const uint32_t HEADER_HEIGHT = 200;
const uint32_t FOOTER_HEIGHT = 600;

const uint32_t FFT_SIZE = 256;
const uint64_t FREQUENCY_START = 10e6;
const uint64_t FREQUENCY_END = 1655e6;
const uint32_t FREQUENCY_STEP = 5e6;
const uint32_t SAMPLE_RATE = 5e6;

// The FREQUENCY_START is the center frequency of each file. The real start of the file is the start - sample_rate / 2.
const uint64_t FREQUENCY_REAL_START = FREQUENCY_START - (SAMPLE_RATE / 2);
const uint64_t FREQUENCY_REAL_END = FREQUENCY_END + (SAMPLE_RATE / 2);
const uint64_t FREQUENCY_REAL_RANGE = FREQUENCY_REAL_END - FREQUENCY_REAL_START;

const uint32_t WIDTH_STEP = FFT_SIZE / (SAMPLE_RATE / FREQUENCY_STEP);
const uint32_t MINOR_TICK_RATE = 1e6;
const double MINOR_TICK_SIZE = (FFT_SIZE / (double) FREQUENCY_STEP) * MINOR_TICK_RATE;
const uint32_t MINOR_TICK_HEIGHT = 30;
const uint32_t MAJOR_TICK_RATE = 50e6;
const double MAJOR_TICK_SIZE = (FFT_SIZE / (double) FREQUENCY_STEP) * MAJOR_TICK_RATE;
const uint32_t MAJOR_TICK_HEIGHT = 60;
const uint8_t LINE_COLOR = 255;
const char* FONT_FILE = "../fonts/RobotoCondensed-Regular.ttf";
const uint16_t FONT_SIZE_PX = 48;

// Utility //////////////////////////////////////////////////////////////////

uint8_t max_u8(uint8_t a, uint8_t b) {
    return a > b ? a : b;
}

// Image operations /////////////////////////////////////////////////////////

void img_gray_copy(uint8_t *dst, uint8_t *src, uint32_t dst_x, uint32_t dst_y, uint32_t src_x, uint32_t src_y, uint32_t width, uint32_t height, uint32_t dst_stride, uint32_t src_stride) {
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            uint32_t dst_offset = (dst_y + i) * dst_stride + dst_x + j;
            uint32_t src_offset = (src_y + i) * src_stride + src_x + j;
            dst[dst_offset] = max_u8(dst[dst_offset], src[src_offset]);
        }
    }
}

void img_pixel_put(uint8_t *buffer, uint32_t stride, uint32_t x, uint32_t y, uint8_t v) {
    if (x > 0 && y > 0) {
        buffer[(y * stride) + x] = v;
    }
}

void img_vline(uint8_t *buffer, uint32_t stride, uint32_t x1, uint32_t y1, uint32_t y2, uint8_t v) {
    if (x1 > stride) return;
    for (int y = y1; y < y2; y++) {
        img_pixel_put(buffer, stride, x1, y, v);
    }
}

void img_hline(uint8_t *buffer, uint32_t stride, uint32_t x1, uint32_t y1, uint32_t x2, uint8_t v) {
    for (int x = x1; x < x2; x++) {
        img_pixel_put(buffer, stride, x, y1, v);
    }
}

// Font operations //////////////////////////////////////////////////////////

typedef struct {
    stbtt_fontinfo font;
    uint8_t *buffer;
} ntt_font;

ntt_font *ntt_font_load(const char *font_file) {
    ntt_font *font = calloc(1, sizeof(ntt_font));

    FILE *fp = fopen(font_file, "rb");
    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    font->buffer = calloc(size, 1);
    fread(font->buffer, size, 1, fp);
    fclose(fp);

    stbtt_InitFont(&font->font, font->buffer, stbtt_GetFontOffsetForIndex(font->buffer, 0));
    return font;
}

void ntt_font_measure(const ntt_font *font, const char *text, const int x, const int y, const int font_size, int *width, int *height) {
    float font_scale = stbtt_ScaleForPixelHeight(&font->font, font_size);
    int ascent, descent;
    stbtt_GetFontVMetrics(&font->font, &ascent, &descent, 0);
    int baseline = (int) (ascent * font_scale);
    int descent_scaled = (int) (descent * font_scale);
    //printf("Baseline %d descent %d\n", baseline, descent_scaled);

    *width = x;
    // The glyph height is constant for the font.
    // Descent_scaled is negative.
    *height = y + (baseline - descent_scaled);

    int ch = 0;
    while (text[ch]) {
        int x0, y0, x1, y1;
        int advance, lsb;
        char c = text[ch];
        stbtt_GetCodepointBitmapBox(&font->font, c, font_scale, font_scale, &x0, &y0, &x1, &y1);
        stbtt_GetCodepointHMetrics(&font->font, c, &advance, &lsb);
        int glyph_width = advance * font_scale;
        *width += glyph_width;
        if (text[ch + 1]) {
            *width += font_scale * stbtt_GetCodepointKernAdvance(&font->font, c, text[ch + 1]);
        }
        ch++;
    }
}

void ntt_font_draw(const ntt_font *font, uint8_t *img, const uint32_t img_stride, const char *text, const int x, const int y, const int font_size) {
    int text_width, text_height;
    ntt_font_measure(font, text, 0, 0, font_size, &text_width, &text_height);
    int start_x = x - text_width / 2;
    if (start_x < 0) return;

    float font_scale = stbtt_ScaleForPixelHeight(&font->font, font_size);
    int ascent;
    stbtt_GetFontVMetrics(&font->font, &ascent, 0, 0);
    int baseline = (int) (ascent * font_scale);

    int ch = 0;
    int _x = 0;
    while (text[ch]) {
        int w, h, dx, dy;
        int advance, lsb;
        char c = text[ch];
        stbtt_GetCodepointHMetrics(&font->font, c, &advance, &lsb);
        uint8_t *glyph_bitmap = stbtt_GetCodepointBitmap(&font->font, font_scale, font_scale, c, &w, &h, &dx, &dy);
        //printf("Offset %d %d\n", dx, baseline + dy);
        img_gray_copy(img, glyph_bitmap, start_x + _x + dx, y + baseline + dy, 0, 0, w, h, img_stride, w);
        //x += w;
        _x += (advance * font_scale);

        if (text[ch + 1]) {
            _x += font_scale * stbtt_GetCodepointKernAdvance(&font->font, c, text[ch + 1]);
        }

        ch++;
    }
}

// Main /////////////////////////////////////////////////////////////////////

int frequency_to_x(const int image_width, const uint64_t freq) {
    const uint64_t real_freq = freq - FREQUENCY_REAL_START;
    return round(real_freq / (double) FREQUENCY_REAL_RANGE * image_width);
}

int main() {
    ntt_font *font = ntt_font_load(FONT_FILE);

    char in_file_name[100];
    snprintf(in_file_name, 100, "broad-stitched-%.0f-%.0f.png", FREQUENCY_START / 1e6, FREQUENCY_END / 1e6);
    char out_file_name[100];
    snprintf(out_file_name, 100, "broad-stitched-%.0f-%.0f-markers.png", FREQUENCY_START / 1e6, FREQUENCY_END / 1e6);

    int width, height, n;
    printf("Reading %s...\n", in_file_name);
    uint8_t *in_buffer = stbi_load(in_file_name, &width, &height, &n, 1);
    int out_width = width;
    int out_height = height + HEADER_HEIGHT + FOOTER_HEIGHT;
    uint8_t *out_buffer = calloc(out_width * out_height, sizeof(uint8_t));

    printf("Composing...\n");
    img_gray_copy(out_buffer, in_buffer, 0, HEADER_HEIGHT, 0, 0, width, height, out_width, width);

    printf("Adding markers...\n");

    int header_bottom = HEADER_HEIGHT;
    int footer_top = HEADER_HEIGHT + height;
    int footer_bottom = out_height - 1;

    // Add white lines at header bottom + footer top/bottom.
    const int border_height = 10;
    for (int i = 0; i < border_height; i++) {
        img_hline(out_buffer, out_width, 0, header_bottom - i, out_width, LINE_COLOR);
        img_hline(out_buffer, out_width, 0, footer_top + i, out_width, LINE_COLOR);
        img_hline(out_buffer, out_width, 0, footer_bottom - i, out_width, LINE_COLOR);
    }
    footer_top += border_height;
    footer_bottom -= border_height;

    // Add minor ticks.
    for (uint64_t freq = 0; freq < FREQUENCY_REAL_END; freq += MINOR_TICK_RATE) {
        int x = frequency_to_x(out_width, freq);
        if (x > 0 && x < out_width) {
            for (int dx = -1; dx <= 1; dx += 1) {
                img_vline(out_buffer, out_width, x + dx, footer_top, footer_top + MINOR_TICK_HEIGHT, LINE_COLOR);
                img_vline(out_buffer, out_width, x + dx, footer_bottom - MINOR_TICK_HEIGHT, footer_bottom, LINE_COLOR);
            }
        }
    }

    // Add major ticks + text.
    const uint32_t labels_y = height + HEADER_HEIGHT + (FOOTER_HEIGHT / 2 - FONT_SIZE_PX / 2);
    for (uint64_t freq = 0; freq < FREQUENCY_REAL_END; freq += MAJOR_TICK_RATE) {
        int x = frequency_to_x(out_width, freq);
        if (x > 0 && x < out_width) {
            for (int dx = -1; dx <= 1; dx += 1) {
                img_vline(out_buffer, out_width, x + dx, footer_top, footer_top + MAJOR_TICK_HEIGHT, LINE_COLOR);
                img_vline(out_buffer, out_width, x + dx, footer_bottom - MAJOR_TICK_HEIGHT, footer_bottom, LINE_COLOR);
            }
            if (freq > FREQUENCY_REAL_START && freq < FREQUENCY_REAL_END) {
                char text[200];
                snprintf(text, 200, "%.2f", (freq / (double) 1e6));
                ntt_font_draw(font, out_buffer, out_width, text, x, labels_y, FONT_SIZE_PX);
            }
        }
    }

    printf("Writing %s...\n", out_file_name);
    write_gray_png(out_file_name, out_width, out_height, out_buffer);

    exit(0);
}

