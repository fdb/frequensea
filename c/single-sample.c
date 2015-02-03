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
const int WIDTH = IQ_RESOLUTION * SIZE_MULTIPLIER;
const int HEIGHT = IQ_RESOLUTION * SIZE_MULTIPLIER;
const int PIXEL_INC = 4;

void pixel_put(uint8_t *image_buffer, int x, int y, int color) {
    int offset = y * WIDTH + x;
    image_buffer[offset] = color;
}

void pixel_inc(uint8_t *image_buffer, int x, int y) {
    static int have_warned = 0;
    int offset = y * WIDTH + x;
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

int main() {
    FILE *fp = fopen("../rfdata/rf-2.500-1.raw", "r");
    assert(fp != NULL);
    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    uint8_t *sample_buffer = calloc(size, sizeof(uint8_t));
    fread(sample_buffer, size, 1, fp);
    fclose(fp);

    uint8_t *image_buffer = calloc(WIDTH * HEIGHT, sizeof(uint8_t));
    printf("Size: %ld\n", size);

    int x1 = 0;
    int y1 = 0;

    int fname_index = 1;
    for (int j = 0; j < size; j += SAMPLES_STEP) {
        memset(image_buffer, 0, WIDTH * HEIGHT * sizeof(uint8_t));
        for (int i = 0; i < j; i += 2) {
            int x2 = (sample_buffer[i] + 128) % 256;
            int y2 = (sample_buffer[i + 1] + 128) % 256;
            if (i > 0) {
                draw_line(image_buffer, x1 * SIZE_MULTIPLIER, y1 * SIZE_MULTIPLIER, x2 * SIZE_MULTIPLIER, y2 * SIZE_MULTIPLIER, 0);
            }
            x1 = x2;
            y1 = y2;
            //printf("%d, %d\n", x, y);
            //pixel_inc(image_buffer, x, y);
        }
        char fname[100];
        snprintf(fname, 100, "sample-%d.png", fname_index);
        write_gray_png(fname, WIDTH, HEIGHT, image_buffer);
        fname_index++;
    }



    return 0;
}
