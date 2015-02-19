#include <stdlib.h>
#include <string.h>

#include "nul.h"

nul_buffer *nul_buffer_new_u8(int width, int height, int channels, const uint8_t *data) {
    nul_buffer *buffer = calloc(1, sizeof(nul_buffer));
    buffer->type = NUL_BUFFER_U8;
    buffer->width = width;
    buffer->height = height;
    buffer->channels = channels;
    buffer->size_bytes = width * height * channels * sizeof(uint8_t);
    buffer->data.u8 = calloc(buffer->size_bytes, 1);
    if (data != NULL) {
        memcpy(buffer->data.u8, data, buffer->size_bytes);
    }
    return buffer;
}

nul_buffer *nul_buffer_new_f64(int width, int height, int channels, const double *data) {
    nul_buffer *buffer = calloc(1, sizeof(nul_buffer));
    buffer->type = NUL_BUFFER_F64;
    buffer->width = width;
    buffer->height = height;
    buffer->channels = channels;
    buffer->size_bytes = width * height * channels * sizeof(double);
    buffer->data.f64 = calloc(buffer->size_bytes, 1);
    if (data != NULL) {
        memcpy(buffer->data.f64, data, buffer->size_bytes);
    }
    return buffer;
}

void nul_buffer_free(nul_buffer *buffer) {
    if (buffer->type == NUL_BUFFER_U8) {
        free(buffer->data.u8);
    } else {
        free(buffer->data.f64);
    }
    free(buffer);
}
