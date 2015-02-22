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

const uint32_t FFT_SIZE = 256;
const uint32_t FFT_HISTORY_SIZE = 4096;
const uint64_t FREQUENCY_START = 10e6;
const uint64_t FREQUENCY_END = 60e6;
const uint32_t FREQUENCY_STEP = 5e6;
const uint32_t SAMPLE_RATE = 5e6;

const uint32_t FREQUENCY_RANGE = (FREQUENCY_END - FREQUENCY_START) / FREQUENCY_STEP;
const uint32_t WIDTH_STEP = FFT_SIZE / (SAMPLE_RATE / FREQUENCY_STEP);
const uint32_t IMAGE_WIDTH = FFT_SIZE + (FREQUENCY_RANGE) * WIDTH_STEP;
const uint32_t IMAGE_HEIGHT = FFT_HISTORY_SIZE;


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

// Main /////////////////////////////////////////////////////////////////////

int main() {
    uint32_t image_height = IMAGE_HEIGHT;
    printf("Image size: %d x %d\n", IMAGE_WIDTH, image_height);
    uint8_t *buffer = calloc(IMAGE_WIDTH * image_height, sizeof(uint8_t));
    uint32_t x = 0;
    for (uint32_t frequency = FREQUENCY_START; frequency <= FREQUENCY_END; frequency += FREQUENCY_STEP) {
        char file_name[100];
        snprintf(file_name, 100, "broad-%.0f.png", frequency / 1.0e6);
        printf("Composing %s...\n", file_name);

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
        img_gray_copy(buffer, image_data, x, 0, 0, 0, FFT_SIZE, FFT_HISTORY_SIZE, IMAGE_WIDTH, FFT_SIZE);

        stbi_image_free(image_data);

        x += WIDTH_STEP;
    }

    char out_file_name[100];
    snprintf(out_file_name, 100, "broad-stitched-%.0f-%.0f.png", FREQUENCY_START / 1e6, FREQUENCY_END / 1e6);
    printf("Saving %s...\n", out_file_name);

    write_gray_png(out_file_name, IMAGE_WIDTH, image_height, buffer);
    exit(0);
}

