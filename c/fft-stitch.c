#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../externals/stb/stb_image.h"

#include "easypng.h"

// Stitch FFT sweeps PNG

const int FFT_SIZE = 2048;
const int FFT_HISTORY_SIZE = 512;
const int FREQUENCY_START = 900.0e6;
const int FREQUENCY_END = 1000.0e6;
const int FREQUENCY_STEP = 10e6;
const int SAMPLE_RATE = 10e6;
const int WIDTH_STEP = FFT_SIZE / (SAMPLE_RATE / FREQUENCY_STEP);

void img_gray_copy(uint8_t *dst, uint8_t *src, int dst_x, int dst_y, int src_x, int src_y, int width, int height, int dst_stride, int src_stride) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            dst[(dst_y + i) * dst_stride + dst_x + j] = src[(src_y + i) * src_stride + src_x + j];
        }
    }
}


int main() {
    int total_height = FFT_HISTORY_SIZE;
    int frequency_range = (FREQUENCY_END - FREQUENCY_START) / FREQUENCY_STEP;
    int total_width = FFT_SIZE + (frequency_range - 1) * WIDTH_STEP;
    uint8_t *buffer = calloc(total_width * total_height, sizeof(uint8_t));
    int x = 0;
    for (int frequency = FREQUENCY_START; frequency <= FREQUENCY_END; frequency += FREQUENCY_STEP) {
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
    write_gray_png("fft-stitched.png", total_width, total_height, buffer);
    exit(0);
}

