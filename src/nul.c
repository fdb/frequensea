#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nul.h"

nul_buffer *nul_buffer_new_u8(int length, int channels, const uint8_t *data) {
    nul_buffer *buffer = calloc(1, sizeof(nul_buffer));
    buffer->type = NUL_BUFFER_U8;
    buffer->length = length;
    buffer->channels = channels;
    buffer->size_bytes = length * channels * sizeof(uint8_t);
    buffer->data.u8 = calloc(buffer->size_bytes, 1);
    if (data != NULL) {
        memcpy(buffer->data.u8, data, buffer->size_bytes);
    }
    return buffer;
}

nul_buffer *nul_buffer_new_f64(int length, int channels, const double *data) {
    nul_buffer *buffer = calloc(1, sizeof(nul_buffer));
    buffer->type = NUL_BUFFER_F64;
    buffer->length = length;
    buffer->channels = channels;
    buffer->size_bytes = length * channels * sizeof(double);
    buffer->data.f64 = calloc(buffer->size_bytes, 1);
    if (data != NULL) {
        memcpy(buffer->data.f64, data, buffer->size_bytes);
    }
    return buffer;
}

nul_buffer *nul_buffer_copy(nul_buffer *buffer) {
    assert(buffer != NULL);
    if (buffer->type == NUL_BUFFER_U8) {
        return nul_buffer_new_u8(buffer->length, buffer->channels, buffer->data.u8);
    } else {
        return nul_buffer_new_f64(buffer->length, buffer->channels, buffer->data.f64);
    }
}

nul_buffer *nul_buffer_reduce(nul_buffer *buffer, double percentage) {
    percentage = percentage < 0.0 ? 0.0 : percentage > 1.0 ? 1.0 : percentage;
    int new_length = round(buffer->length * percentage);
    assert(buffer != NULL);
    if (buffer->type == NUL_BUFFER_U8) {
        return nul_buffer_new_u8(new_length, buffer->channels, buffer->data.u8);
    } else {
        return nul_buffer_new_f64(new_length, buffer->channels, buffer->data.f64);
    }
}

void nul_buffer_set_data(nul_buffer *dst, nul_buffer *src) {
    assert(dst != NULL);
    assert(src != NULL);
    assert(dst->type == src->type);
    assert(dst->size_bytes == src->size_bytes);
    if (dst->type == NUL_BUFFER_U8) {
        memcpy(dst->data.u8, src->data.u8, dst->size_bytes);
    } else {
        memcpy(dst->data.f64, src->data.f64, dst->size_bytes);
    }
}

void nul_buffer_append(nul_buffer *dst, nul_buffer *src) {
    assert(dst != NULL);
    assert(src != NULL);
    assert(dst->type == src->type);
    int dst_size = dst->length * dst->channels;
    int src_size = src->length * src->channels;
    int new_size = dst_size + src_size;
    if (dst->type == NUL_BUFFER_U8) {
        uint8_t *new_data = calloc(new_size, sizeof(uint8_t));
        memcpy(new_data, dst->data.u8, dst->size_bytes);
        memcpy(new_data + dst_size, src->data.u8, src->size_bytes);
        free(dst->data.u8);
        dst->data.u8 = new_data;
        dst->size_bytes = new_size * sizeof(uint8_t);
    } else {
        double *new_data = calloc(new_size, sizeof(double));
        memcpy(new_data, dst->data.f64, dst->size_bytes);
        memcpy(new_data + dst_size, src->data.f64, src->size_bytes);
        free(dst->data.f64);
        dst->data.f64 = new_data;
        dst->size_bytes = new_size * sizeof(double);
    }
    dst->length = dst->length + src->length;
}

uint8_t nul_buffer_get_u8(nul_buffer *buffer, int offset) {
    if (buffer->type == NUL_BUFFER_U8) {
        return buffer->data.u8[offset];
    } else {
        return buffer->data.f64[offset] * 256.0;
    }
}

double nul_buffer_get_f64(nul_buffer *buffer, int offset) {
    if (buffer->type == NUL_BUFFER_U8) {
        return buffer->data.u8[offset] / 256.0;
    } else {
        return buffer->data.f64[offset];
    }
}

void nul_buffer_set_u8(nul_buffer *buffer, int offset, uint8_t value) {
    if (buffer->type == NUL_BUFFER_U8) {
        buffer->data.u8[offset] = value;
    } else {
        buffer->data.f64[offset] = value / 256.0;
    }
}

void nul_buffer_set_f64(nul_buffer *buffer, int offset, double value) {
    if (buffer->type == NUL_BUFFER_U8) {
        buffer->data.u8[offset] = value * 256.0;
    } else {
        buffer->data.f64[offset] = value;
    }
}

nul_buffer *nul_buffer_convert(nul_buffer *buffer, nul_buffer_type new_type) {
    assert(buffer != NULL);
    int size = buffer->length * buffer->channels;
    if (new_type == NUL_BUFFER_U8) {
        nul_buffer *out = nul_buffer_new_u8(buffer->length, buffer->channels, NULL);
        uint8_t *out_data = out->data.u8;
        for (int i = 0; i < size; i++) {
            out_data[i] = nul_buffer_get_u8(buffer, i);
        }
        return out;
    } else {
        nul_buffer *out = nul_buffer_new_f64(buffer->length, buffer->channels, NULL);
        double *out_data = out->data.f64;
        for (int i = 0; i < size; i++) {
            out_data[i] = nul_buffer_get_f64(buffer, i);
        }
        return out;
    }
}

void nul_buffer_save(nul_buffer *buffer, const char *fname) {
    assert(buffer != NULL);
    FILE *fp = fopen(fname, "wb");
    if (fp) {
        fwrite(buffer->data.u8, buffer->size_bytes, 1, fp);
        fclose(fp);
        printf("Written %s.\n", fname);
    }
}

void nul_buffer_free(nul_buffer *buffer) {
    if (buffer->type == NUL_BUFFER_U8) {
        free(buffer->data.u8);
    } else {
        free(buffer->data.f64);
    }
    free(buffer);
}
