// Slowly show a single sample, frame by frame. Used for exporting to movie.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "easypng.h"

const int SAMPLES_STEP = 100;
const int WIDTH = 256;
const int HEIGHT = 256;
const int PIXEL_INC = 3;

void pixel_put(uint8_t *image_buffer, int x, int y, int color) {
    int offset = y * WIDTH + x;
    image_buffer[offset] = color;
}

void pixel_inc(uint8_t *image_buffer, int x, int y) {
    int offset = y * WIDTH + x;
    int v = image_buffer[offset];
    if (v + PIXEL_INC >= 255) {
        fprintf(stderr, "WARN: pixel value out of range (%d, %d)\n", x, y);
    } else {
        v += PIXEL_INC;
        image_buffer[offset] = v;
    }
}

void draw_line(uint8_t *image_buffer, int x1, int y1, int x2, int y2, int color) {
    static int num_levels = 256;
    static int intensity_bits = 8;
    unsigned short intensity_shift, error_adj, error_acc;
   unsigned short error_acc_temp, weighting, weighting_complement_mask;
   short delta_x, delta_y, tmp, x_dir;

   /* Make sure the line runs top to bottom */
   if (y1 > y2) {
      tmp = y1; y1 = y2; y2 = tmp;
      tmp = x1; x1 = x2; x2 = tmp;
   }
   /* Draw the initial pixel, which is always exactly intersected by
      the line and so needs no weighting */
   pixel_put(image_buffer, x1, y1, color);

   if ((delta_x = x2 - x1) >= 0) {
      x_dir = 1;
   } else {
      x_dir = -1;
      delta_x = -delta_x; /* make delta_x positive */
   }
   /* Special-case horizontal, vertical, and diagonal lines, which
      require no weighting because they go right through the center of
      every pixel */
   if ((delta_y = y2 - y1) == 0) {
      /* Horizontal line */
      while (delta_x-- != 0) {
         x1 += x_dir;
         pixel_put(image_buffer, x1, y1, color);
      }
      return;
   }
   if (delta_x == 0) {
      /* Vertical line */
      do {
         y1++;
         pixel_put(image_buffer,x1, y1, color);
      } while (--delta_y != 0);
      return;
   }
   if (delta_x == delta_y) {
      /* Diagonal line */
      do {
         x1 += x_dir;
         y1++;
         pixel_put(image_buffer, x1, y1, color);
      } while (--delta_y != 0);
      return;
   }
   /* Line is not horizontal, diagonal, or vertical */
   error_acc = 0;  /* initialize the line error accumulator to 0 */
   /* # of bits by which to shift error_acc to get intensity level */
   intensity_shift = 16 - intensity_bits;
   /* Mask used to flip all bits in an intensity weighting, producing the
      result (1 - intensity weighting) */
   weighting_complement_mask = num_levels - 1;
   /* Is this an X-major or Y-major line? */
   if (delta_y > delta_x) {
      /* Y-major line; calculate 16-bit fixed-point fractional part of a
         pixel that X advances each time Y advances 1 pixel, truncating the
         result so that we won't overrun the endpoint along the X axis */
      error_adj = ((unsigned long) delta_x << 16) / (unsigned long) delta_y;
      /* Draw all pixels other than the first and last */
      while (--delta_y) {
         error_acc_temp = error_acc;   /* remember currrent accumulated error */
         error_acc += error_adj;      /* calculate error for next pixel */
         if (error_acc <= error_acc_temp) {
            /* The error accumulator turned over, so advance the X coord */
            x1 += x_dir;
         }
         y1++; /* Y-major, so always advance Y */
         /* The intensity_bits most significant bits of error_acc give us the
            intensity weighting for this pixel, and the complement of the
            weighting for the paired pixel */
         weighting = error_acc >> intensity_shift;
         pixel_put(image_buffer, x1, y1, color + weighting);
         pixel_put(image_buffer, x1 + x_dir, y1,
               color + (weighting ^ weighting_complement_mask));
      }
      /* Draw the final pixel, which is
         always exactly intersected by the line
         and so needs no weighting */
      pixel_put(image_buffer, x2, y2, color);
      return;
   }
   /* It's an X-major line; calculate 16-bit fixed-point fractional part of a
      pixel that Y advances each time X advances 1 pixel, truncating the
      result to avoid overrunning the endpoint along the X axis */
   error_adj = ((unsigned long) delta_y << 16) / (unsigned long) delta_x;
   /* Draw all pixels other than the first and last */
   while (--delta_x) {
      error_acc_temp = error_acc;   /* remember currrent accumulated error */
      error_acc += error_adj;      /* calculate error for next pixel */
      if (error_acc <= error_acc_temp) {
         /* The error accumulator turned over, so advance the Y coord */
         y1++;
      }
      x1 += x_dir; /* X-major, so always advance X */
      /* The intensity_bits most significant bits of error_acc give us the
         intensity weighting for this pixel, and the complement of the
         weighting for the paired pixel */
      weighting = error_acc >> intensity_shift;
      pixel_put(image_buffer, x1, y1, color + weighting);
      pixel_put(image_buffer, x1, y1 + 1,
            color + (weighting ^ weighting_complement_mask));
   }
   /* Draw the final pixel, which is always exactly intersected by the line
      and so needs no weighting */
   pixel_put(image_buffer, x2, y2, color);
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
        memset(image_buffer, 255, WIDTH * HEIGHT * sizeof(uint8_t));
        for (int i = 0; i < j; i += 2) {
            int x2 = (sample_buffer[i] + 128) % 256;
            int y2 = (sample_buffer[i + 1] + 128) % 256;
            if (i > 0) {
                draw_line(image_buffer, x1, y1, x2, y2, 0);
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
