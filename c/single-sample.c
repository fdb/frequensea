// Slowly show a single sample, frame by frame. Used for exporting to movie.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "easypng.h"

const int SAMPLES_STEP = 100;
const int IQ_RESOLUTION = 256;
const int SIZE_MULTIPLIER = 4;
const int IMAGE_WIDTH = 1920;
const int IMAGE_HEIGHT = 1080;
const int IQ_WIDTH = IQ_RESOLUTION * SIZE_MULTIPLIER;
const int IQ_HEIGHT = IQ_RESOLUTION * SIZE_MULTIPLIER;
const int IMAGE_OFFSET_X = (IMAGE_WIDTH - IQ_WIDTH) / 2;
const int IMAGE_OFFSET_Y = (IMAGE_HEIGHT - IQ_HEIGHT) / 2;
const int PIXEL_INC = 4;

uint8_t clamp_u8(int v, uint8_t min, uint8_t max) {
    return (uint8_t) (v < min ? min : v > max ? max : v);
}

void pixel_put(uint8_t *image_buffer, int x, int y, int color) {
    int offset = y * IQ_WIDTH + x;
    image_buffer[offset] = color;
}

void pixel_inc(uint8_t *image_buffer, int x, int y) {
    static int have_warned = 0;
    // Avoid white borders.
    if (x == 0 || y == 0 || x == IQ_WIDTH - 1 || y == IQ_HEIGHT - 1) {
        return;
    }
    int offset = ((y + IMAGE_OFFSET_Y) * IMAGE_WIDTH) + (x + IMAGE_OFFSET_X);
    int v = image_buffer[offset];
    if (v + PIXEL_INC >= 255) {
        if (!have_warned) {
            fprintf(stderr, "WARN: pixel value out of range (%d, %d)\n", x, y);
            have_warned = 1;
        }
    } else {
        v += PIXEL_INC;
        image_buffer[offset] = v;
    }
}

void draw_line(uint8_t *image_buffer, int x1, int y1, int x2, int y2, int color) {
  int dx = abs(x2 - x1);
  int sx = x1 < x2 ? 1 : -1;
  int dy = abs(y2-y1);
  int sy = y1 < y2 ? 1 : -1;
  int err = (dx > dy ? dx : -dy) / 2;
  int e2;

  for(;;){
    pixel_inc(image_buffer, x1, y1);
    if (x1 == x2 && y1 == y2) break;
    e2 = err;
    if (e2 > -dx) { err -= dy; x1 += sx; }
    if (e2 <  dy) { err += dx; y1 += sy; }
  }
}

void write_image(uint8_t *image_buffer, int fname_index) {
    char fname[100];
    snprintf(fname, 100, "_export/sample-%d.png", fname_index);
    write_gray_png(fname, IMAGE_WIDTH, IMAGE_HEIGHT, image_buffer);
}

int main() {
    FILE *fp = fopen("../rfdata/rf-612.004-1.raw", "r");
    assert(fp != NULL);
    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    uint8_t *sample_buffer = calloc(size, sizeof(uint8_t));
    fread(sample_buffer, size, 1, fp);
    fclose(fp);

    uint8_t *image_buffer = calloc(IMAGE_WIDTH * IMAGE_HEIGHT, sizeof(uint8_t));
    printf("Size: %ld\n", size);

    int x1 = 0;
    int y1 = 0;

    int fname_index = 1;
    for (int j = 0; j < size; j += SAMPLES_STEP) {
        for (int i = 0; i < SAMPLES_STEP; i += 2) {
            int x2 = (sample_buffer[j + i] + 128) % 256;
            int y2 = (sample_buffer[j + i + 1] + 128) % 256;
            if (i > 0) {
                draw_line(image_buffer, x1 * SIZE_MULTIPLIER, y1 * SIZE_MULTIPLIER, x2 * SIZE_MULTIPLIER, y2 * SIZE_MULTIPLIER, 0);
            }
            x1 = x2;
            y1 = y2;
        }
        write_image(image_buffer, fname_index);
        fname_index++;
    }

    return 0;
}
