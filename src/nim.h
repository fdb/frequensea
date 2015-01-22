// Image operations

#ifndef NIM_H
#define NIM_H

#include <stdint.h>
#include <png.h>

typedef enum {
    NIM_GRAY = PNG_COLOR_TYPE_GRAY,
    NIM_RGB = PNG_COLOR_TYPE_RGB
} nim_color_mode;

void nim_png_write(const char *fname, int width, int height, nim_color_mode mode, uint8_t *buffer);

#endif // NIM_H
