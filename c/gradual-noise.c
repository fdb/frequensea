// Gradual noise movie.
// Expects noise data files in ../rfdata/rf-x.xxx-big.raw

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "easypng.h"

const int BLOCK_SIZE_BYTES = 262144;
const int IMAGE_WIDTH = 1920;
const int IMAGE_HEIGHT = 1080;
const int IQ_SIZE = 256;
const int RF_BUFFER_SIZE = IMAGE_WIDTH * IMAGE_HEIGHT * 2;
const int IMAGE_BUFFER_SIZE = IMAGE_WIDTH * IMAGE_HEIGHT;
const int TOTAL_FRAMES = 1000;
const double INTERPOLATE_STEP = 0.1;
const double FREQ_MHZ_START = 1.0;
const double FREQ_MHZ_STEP = 0.01;
const double WIDTH_SCALE = IMAGE_WIDTH / (double) IQ_SIZE;
const double HEIGHT_SCALE = IMAGE_HEIGHT / (double) IQ_SIZE;
const double BLOCK_SCALE  = WIDTH_SCALE > HEIGHT_SCALE ? WIDTH_SCALE : HEIGHT_SCALE;

// Modeled after the piecewise quadratic
// y = (1/2)((2x)^2)             ; [0, 0.5)
// y = -(1/2)((2x-1)*(2x-3) - 1) ; [0.5, 1]
double quadratic_ease_in_out(double p) {
    if(p < 0.5) {
        return 2 * p * p;
    } else {
        return (-2 * p * p) + (4 * p) - 1;
    }
}

// Modeled after the piecewise quintic
// y = (1/2)((2x)^5)       ; [0, 0.5)
// y = (1/2)((2x-2)^5 + 2) ; [0.5, 1]
double quintic_ease_in_out(double p) {
    if (p < 0.5) {
        return 16 * p * p * p * p * p;
    } else {
        double f = ((2 * p) - 2);
        return  0.5 * f * f * f * f * f + 1;
    }
}

double lerp(double a, double b, double t) {
    t = quintic_ease_in_out(t);
    return a * (1.0 - t) + b * t;
}

double clamp(double v, double min, double max) {
    return v < min ? min : v > max ? max : v;
}

void read_buffer(uint8_t *dst, double freq_mhz) {
    char fname[100];
    snprintf(fname, 100, "../rftmp/rf-%.3f-big.raw", freq_mhz);
    FILE *fp = fopen(fname, "r");
    fread(dst, RF_BUFFER_SIZE, 1, fp);
    fclose(fp);
}

static inline void put_pixel(uint8_t *image_buffer, int x, int y, uint8_t color) {
    if (x >= IMAGE_WIDTH || y >= IMAGE_HEIGHT) return;
    int offset = y * IMAGE_WIDTH + x;
    image_buffer[offset] = color;
}

static inline void put_block(uint8_t *image_buffer, int x, int y, uint8_t color) {
    for (int dy = 0; dy < BLOCK_SCALE; dy++) {
        for (int dx = 0; dx < BLOCK_SCALE; dx++) {
            put_pixel(image_buffer, x * BLOCK_SCALE + dx, y * BLOCK_SCALE + dy, color);
        }
    }
}

int main() {
    double freq_mhz = FREQ_MHZ_START;
    double t = 0.0;

    uint8_t *rf_buffer_a = calloc(RF_BUFFER_SIZE, 1);
    uint8_t *rf_buffer_b = calloc(RF_BUFFER_SIZE, 1);
    uint8_t *image_buffer = calloc(IMAGE_BUFFER_SIZE, 1);

    read_buffer(rf_buffer_a, freq_mhz);
    read_buffer(rf_buffer_b, freq_mhz + FREQ_MHZ_STEP);

    for (int frame = 1; frame <= TOTAL_FRAMES; frame++) {
        int i = 0;
        for (int y = 0; y < IQ_SIZE; y++) {
            for (int x = 0; x < IQ_SIZE; x++) {
                int ai = (rf_buffer_a[i] + 128) % 256;
                int bi = (rf_buffer_b[i] + 128) % 256;
                double pwr = lerp(ai, bi, t);
                int color = clamp(pwr, 0, 255);
                put_block(image_buffer, x, y, color);
                i += 2;
            }
        }

        char fname[100];
        snprintf(fname, 100, "_export/noise-%d.png", frame);
        write_gray_png(fname, IMAGE_WIDTH, IMAGE_HEIGHT, image_buffer);

        t += INTERPOLATE_STEP;

        if (t >= 1.0) {
            t = 0.0;
            freq_mhz += FREQ_MHZ_STEP;
            printf("Frequency: %.3f\n", freq_mhz);
            read_buffer(rf_buffer_a, freq_mhz);
            read_buffer(rf_buffer_b, freq_mhz + FREQ_MHZ_STEP);
        }
    }
}
