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

// Stitch FFT sweeps PNG

const uint32_t TOTAL_HEIGHT = 11811;
const uint32_t FOOTER_HEIGHT = 600;
const uint32_t FFT_SIZE = 1024;
const uint32_t FFT_HISTORY_SIZE = TOTAL_HEIGHT - FOOTER_HEIGHT;
const uint64_t FREQUENCY_START = 2e6;
const uint64_t FREQUENCY_END = 100e6;
const uint32_t FREQUENCY_STEP = 2e6;
const uint32_t SAMPLE_RATE = 5e6;
const uint32_t WIDTH_STEP = FFT_SIZE / (SAMPLE_RATE / FREQUENCY_STEP);
const uint32_t MINOR_TICK_RATE = 0.1e6;
const double MINOR_TICK_SIZE = (FFT_SIZE / (double) FREQUENCY_STEP / 2) * MINOR_TICK_RATE;
const uint32_t MAJOR_TICK_RATE = 1e6;
const double MAJOR_TICK_SIZE = (FFT_SIZE / (double) FREQUENCY_STEP / 2) * MAJOR_TICK_RATE;
const uint8_t LINE_COLOR = 200;
const char* FONT_FILE = "../fonts/RobotoCondensed-Light.ttf";
const uint16_t FONT_SIZE_PX = 48;
const uint32_t MARKERS_Y = FFT_HISTORY_SIZE + (FOOTER_HEIGHT / 2 - FONT_SIZE_PX / 2);
const uint32_t MARKERS_X = FFT_SIZE-SAMPLE_RATE / 2;

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

int main() {
    ntt_font *font = ntt_font_load(FONT_FILE);
    uint32_t frequency_range = (FREQUENCY_END - FREQUENCY_START) / FREQUENCY_STEP;
    uint32_t image_width = FFT_SIZE + (frequency_range) * WIDTH_STEP;
    uint32_t image_height = TOTAL_HEIGHT;
    printf("Image size: %d x %d\n", image_width, image_height);
    uint8_t *buffer = calloc(image_width * image_height, sizeof(uint8_t));
    //memset(buffer, 128, image_width * image_height);
    uint32_t x = 0;
    for (uint32_t frequency = FREQUENCY_START; frequency <= FREQUENCY_END; frequency += FREQUENCY_STEP) {
        char file_name[100];
        snprintf(file_name, 100, "fft-%.4f.png", frequency / 1.0e6);

        int width, height, n;
        unsigned char *image_data = stbi_load(file_name, &width, &height, &n, 1);
        if (!image_data) {
            fprintf (stderr, "ERROR: could not load %s\n", file_name);
            exit(1);
        }
        if (width != FFT_SIZE || height < FFT_HISTORY_SIZE) {
            fprintf (stderr, "ERROR: bad image size %s\n", file_name);
            exit(1);
        }

        // Compose image into buffer
        img_gray_copy(buffer, image_data, x, 0, 0, 0, FFT_SIZE, FFT_HISTORY_SIZE, image_width, FFT_SIZE);
        x += WIDTH_STEP;
    }

    int banner_y = FFT_HISTORY_SIZE;
    int banner_bottom = image_height;
    for (int i = 0; i < 10; i++) {
        img_hline(buffer, image_width, 0, banner_y++, image_width, LINE_COLOR);
        img_hline(buffer, image_width, 0, banner_bottom--, image_width, LINE_COLOR);
    }

    for (double x = 0; x < image_width; x += MINOR_TICK_SIZE) {
        img_vline(buffer, image_width, x, banner_y, banner_y + 50, LINE_COLOR);
        img_vline(buffer, image_width, x, banner_bottom - 50, banner_bottom, LINE_COLOR);
    }

    int freq = FREQUENCY_START - (SAMPLE_RATE / 2) + (MAJOR_TICK_RATE / 2);
    double start_x = FFT_SIZE / (double) SAMPLE_RATE * (MAJOR_TICK_RATE / 2);
    for (double x = start_x; x < image_width; x += MAJOR_TICK_SIZE) {
        img_vline(buffer, image_width, x, banner_y, banner_y + 100, LINE_COLOR);
        img_vline(buffer, image_width, x, banner_bottom - 100, banner_bottom, LINE_COLOR);
        if (freq >= 0 && freq < FREQUENCY_END + (SAMPLE_RATE / 2)) {
            char text[200];
            snprintf(text, 200, "%.2f", (freq / (double) 1e6));
            ntt_font_draw(font, buffer, image_width, text, x, MARKERS_Y, FONT_SIZE_PX);
        }
        freq += MAJOR_TICK_RATE;
    }

    char out_file_name[100];
    snprintf(out_file_name, 100, "fft-stitched-%.4f-%.4f.png", FREQUENCY_START / 1e6, FREQUENCY_END / 1e6);
    write_gray_png(out_file_name, image_width, image_height, buffer);
    exit(0);
}

