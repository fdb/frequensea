// Gradual noise movie.
// Expects noise data files in ../rfdata/rf-x.xxx-big.raw

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "easypng.h"

const int BLOCK_SIZE_BYTES = 262144;
const int IMAGE_WIDTH = 480;
const int IMAGE_HEIGHT = 270;
const int RF_BUFFER_SIZE = IMAGE_WIDTH * IMAGE_HEIGHT * 2;
const int IMAGE_BUFFER_SIZE = IMAGE_WIDTH * IMAGE_HEIGHT;
const int TOTAL_FRAMES = 1000;
const double INTERPOLATE_STEP = 0.01;
const double FREQ_MHZ_START = 1.0;
const double FREQ_MHZ_STEP = 0.01;

double lerp(double a, double b, double t) {
    return a * (1.0 - t) + b * t;
}

double clamp(double v, double min, double max) {
    return v < min ? min : v > max ? max : v;
}

void read_buffer(uint8_t *dst, double freq_mhz) {
    char fname[100];
    snprintf(fname, 100, "../rfdata/rf-%.3f-big.raw", freq_mhz);
    FILE *fp = fopen(fname, "r");
    fread(dst, RF_BUFFER_SIZE, 1, fp);
    fclose(fp);
}

int main() {
    double freq_mhz = FREQ_MHZ_START;
    double t = 0.0;

    uint8_t *rf_buffer_a = calloc(RF_BUFFER_SIZE, 1);
    uint8_t *rf_buffer_b = calloc(RF_BUFFER_SIZE, 1);
    uint8_t *image_buffer = calloc(IMAGE_BUFFER_SIZE, 1);

    read_buffer(rf_buffer_a, freq_mhz);
    read_buffer(rf_buffer_b, freq_mhz + FREQ_MHZ_STEP);

    int seed = 1;
    for (int frame = 1; frame <= TOTAL_FRAMES; frame++) {

        srand48(seed);
        int j = 0;
        for (int i=0; i < RF_BUFFER_SIZE; i += 2) {
            int ai = (rf_buffer_a[i] + 128) % 256;
            int aq = (rf_buffer_a[i + 1] + 128) % 256;
            int bi = (rf_buffer_b[i] + 128) % 256;
            int bq = (rf_buffer_b[i + 1] + 128) % 256;
            double a_pwr = sqrt(ai * ai + aq * aq) * 0.8;
            double b_pwr = sqrt(bi * bi + bq * bq) * 0.8;
            double pwr = lerp(a_pwr, b_pwr, t);
            image_buffer[j] = clamp(pwr, 0, 255);
            j++;
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
