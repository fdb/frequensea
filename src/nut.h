// Utility

#ifndef NUT_H
#define NUT_H

#include <stdint.h>

// Sleep

void nut_sleep_milliseconds(int millis);

// Buffer

typedef enum {
    NUT_BUFFER_U8 = 1,
    NUT_BUFFER_F64
} nut_buffer_type;

typedef union nut_buffer_data {
    uint8_t *u8;
    double *f64;
} nut_buffer_data;

typedef struct {
    nut_buffer_type type;
    int length;
    int channels;
    int size_bytes;
    nut_buffer_data data;
} nut_buffer;

nut_buffer *nut_buffer_new_u8(int length, int channels, const uint8_t *data);
nut_buffer *nut_buffer_new_f64(int length, int channels, const double *data);
nut_buffer *nut_buffer_copy(nut_buffer *buffer);
nut_buffer *nut_buffer_reduce(nut_buffer *buffer, double percentage);
nut_buffer *nut_buffer_clip(nut_buffer *buffer, int offset, int length);
void nut_buffer_set_data(nut_buffer *dst, nut_buffer *src);
void nut_buffer_append(nut_buffer *dst, nut_buffer *src);
uint8_t nut_buffer_get_u8(nut_buffer *buffer, int offset);
double nut_buffer_get_f64(nut_buffer *buffer, int offset);
void nut_buffer_set_u8(nut_buffer *buffer, int offset, uint8_t value);
void nut_buffer_set_f64(nut_buffer *buffer, int offset, double value);
nut_buffer *nut_buffer_convert(nut_buffer *buffer, nut_buffer_type new_type);
void nut_buffer_save(nut_buffer *buffer, const char *fname);
void nut_buffer_free(nut_buffer *buffer);

#endif // NUT_H
