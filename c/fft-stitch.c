#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../externals/stb/stb_image.h"

#include "easypng.h"

// Stitch FFT sweeps PNG

const uint32_t FFT_SIZE = 1024;
const uint32_t FFT_HISTORY_SIZE = 16384;
const uint64_t FREQUENCY_START = 150e6;
const uint64_t FREQUENCY_END = 320e6;
const uint32_t FREQUENCY_STEP = 2e6;
const uint32_t SAMPLE_RATE = 5e6;
const uint32_t WIDTH_STEP = FFT_SIZE / (SAMPLE_RATE / FREQUENCY_STEP);

void img_gray_copy(uint8_t *dst, uint8_t *src, uint32_t dst_x, uint32_t dst_y, uint32_t src_x, uint32_t src_y, uint32_t width, uint32_t height, uint32_t dst_stride, uint32_t src_stride) {
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            dst[(dst_y + i) * dst_stride + dst_x + j] = src[(src_y + i) * src_stride + src_x + j];
        }
    }
}


int main() {
    uint32_t total_height = FFT_HISTORY_SIZE;
    uint32_t frequency_range = (FREQUENCY_END - FREQUENCY_START) / FREQUENCY_STEP;
    uint32_t total_width = FFT_SIZE + (frequency_range - 1) * WIDTH_STEP;
    uint8_t *buffer = calloc(total_width * total_height, sizeof(uint8_t));
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
        if (width != FFT_SIZE || height != FFT_HISTORY_SIZE) {
            fprintf (stderr, "ERROR: bad image size %s\n", file_name);
            exit(1);
        }

        // Compose image into buffer
        img_gray_copy(buffer, image_data, x, 0, 0, 0, FFT_SIZE, FFT_HISTORY_SIZE, total_width, FFT_SIZE);
        x += WIDTH_STEP;
    }
    char out_file_name[100];
    snprintf(out_file_name, 100, "fft-stiched-%.4f-%.4f.png", FREQUENCY_START / 1.0e6, FREQUENCY_END / 1.0e6);
    write_gray_png(out_file_name, total_width, total_height, buffer);
    exit(0);
}

