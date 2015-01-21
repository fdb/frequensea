#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "nim.h"

// Write a PNG image.
void nim_png_write(const char *fname, int width, int height, nim_color_mode color_mode, uint8_t *buffer) {
    assert(color_mode == NIM_GRAY || color_mode == NIM_RGB);
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_bytepp row_pointers;

    FILE *fp = fopen(fname, "wb");
    if (!fp) {
        printf("ERROR: Could not write open file %s for writing.\n", fname);
        return;
    }

    // Init PNG writer.
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        printf("ERROR: png_create_write_struct.\n");
        return;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        printf("ERROR: png_create_info_struct.\n");
        png_destroy_write_struct(&png_ptr, NULL);
        return;
    }

    png_set_IHDR(png_ptr, info_ptr,
                 width, height,
                 8,
                 color_mode,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    int channels;
    if (color_mode == NIM_GRAY) {
        channels = 1;
    } else if (color_mode == NIM_RGB) {
        channels = 3;
    }

    // PNG expects a list of pointers. We just calculate offsets into our buffer.
    row_pointers = (png_bytepp) png_malloc(png_ptr, height * sizeof(png_bytep));
    for (int y = 0; y < height; y++) {
        // The buffer coming from glReadPixels is upside down. Flip the rows.
        int flipped_y = height - y - 1;
        row_pointers[y] = buffer + (flipped_y * width * channels);
    }

    // Write out the image data.
    png_init_io(png_ptr, fp);
    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    // Cleanup.
    png_free(png_ptr, row_pointers);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    printf("Written %s.\n", fname);
}
