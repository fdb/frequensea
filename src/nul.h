// Utility

#ifndef NUL_H
#define NUL_H

#include <stdint.h>

// Buffer

typedef enum {
    NUL_BUFFER_U8 = 1,
    NUL_BUFFER_F64
} nul_buffer_type;

typedef union nul_buffer_data {
    uint8_t *u8;
    double *f64;
} nul_buffer_data;

typedef struct {
    nul_buffer_type type;
    int width;
    int height;
    int channels;
    int size_bytes;
    nul_buffer_data data;
} nul_buffer;

nul_buffer *nul_buffer_new_u8(int width, int height, int channels, const uint8_t *data);
nul_buffer *nul_buffer_new_f64(int width, int height, int channels, const double *data);
uint8_t nul_buffer_get_u8(nul_buffer *buffer, int offset);
double nul_buffer_get_f64(nul_buffer *buffer, int offset);
void nul_buffer_change_size(nul_buffer *buffer, int width, int height, int channels);
void nul_buffer_free(nul_buffer *buffer);

#endif // NUL_H
