// NDBX Radio Frequency functions, based on HackRF

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nrf.h"

// FIXME: can be removed?
hackrf_device *device;

static void _nrf_buffer_init(nrf_buffer *buffer, int length) {
    printf("init length %d\n", length);
    buffer->length = length + 1;
    buffer->start = 0;
    buffer->end = 0;
    buffer->samples = calloc(buffer->length, sizeof(vec3));
}

static void _nrf_buffer_destroy(nrf_buffer *buffer) {
    free(buffer->samples);
}

static int _nrf_buffer_available_data(nrf_buffer *buffer) {
    return ((buffer->end + 1) % buffer->length) - buffer->start - 1;
}

// static int _nrf_buffer_available_space(nrf_buffer *buffer) {
//     return buffer->length - buffer->end - 1;
// }

// static void *_nrf_buffer_start_ptr(nrf_buffer *buffer) {
//     return buffer->samples + buffer->start * sizeof(vec3);
// }

// static void *_nrf_buffer_end_ptr(nrf_buffer *buffer) {
//     return buffer->samples + buffer->end * sizeof(vec3);
// }

static void _nrf_buffer_commit_write(nrf_buffer *buffer, int length) {
    buffer->end = (buffer->end + length) % buffer->length;
}

static void _nrf_buffer_commit_read(nrf_buffer *buffer, int length) {
    buffer->start = (buffer->start + length) % buffer->length;
}

static void _nrf_buffer_write(nrf_buffer *buffer, const vec3 sample) {
    if (_nrf_buffer_available_data(buffer) == 0) {
        buffer->start = buffer->end = 0;
    }
    //assert(_nrf_buffer_available_space(buffer) >= 1);
    buffer->samples[buffer->end] = sample;
    //void *result = memcpy(_nrf_buffer_end_ptr(buffer), &sample, sizeof(vec3));
    //assert(result != NULL);
    _nrf_buffer_commit_write(buffer, 1);
}

static vec3 _nrf_buffer_read(nrf_buffer *buffer) {
    if (_nrf_buffer_available_data(buffer) < 1) {
        return vec3_init(0, 0, 0);
    } else {
        vec3 result = buffer->samples[buffer->start];
        _nrf_buffer_commit_read(buffer, 1);
        if (buffer->end == buffer->start) {
            buffer->start = buffer->end = 0;
        }
        return result;
    }
}

#define HACKRF_CHECK_STATUS(device, status, message) \
if (status != 0) { \
    printf("FAIL: %s\n", message); \
    hackrf_close(device->device); \
    hackrf_exit(); \
    exit(-1); \
} \

static int receive_sample_block(hackrf_transfer *transfer) {
    nrf_device *device = (nrf_device *)transfer->rx_ctx;
    for (int i = 0; i < transfer->valid_length; i += 2) {
        float t = i / (float) transfer->valid_length;
        int vi = transfer->buffer[i];
        int vq = transfer->buffer[i + 1];
        vi = (vi + 128) % 256;
        vq = (vq + 128) % 256;
        _nrf_buffer_write(&device->buffer, vec3_init(vi / 256.0f, vq / 256.0f, t));
        //device->buffer[device->buffer_length++] =
        //device->samples[j++] = vi / 256.0f;
        //device->samples[j++] = vq / 256.0f;
        //device->samples[j++] = t;
    }
    return 0;
}

// Start receiving on the given frequency.
// If the device could not be opened, use the raw contents of the data_file
// instead.
nrf_device *nrf_start(double freq_mhz, const char* data_file) {
    int status;

    nrf_device *device = malloc(sizeof(nrf_device));
    _nrf_buffer_init(&device->buffer, NRF_SAMPLES_SIZE); // NRF_SAMPLES_SIZE

    status = hackrf_init();
    HACKRF_CHECK_STATUS(device, status, "hackrf_init");

    status = hackrf_open(&device->device);
    if (status == 0) {
        status = hackrf_set_freq(device->device, freq_mhz * 1e6);
        HACKRF_CHECK_STATUS(device, status, "hackrf_set_freq");

        status = hackrf_set_sample_rate(device->device, 5e6);
        HACKRF_CHECK_STATUS(device, status, "hackrf_set_sample_rate");

        status = hackrf_set_amp_enable(device->device, 0);
        HACKRF_CHECK_STATUS(device, status, "hackrf_set_amp_enable");

        status = hackrf_set_lna_gain(device->device, 32);
        HACKRF_CHECK_STATUS(device, status, "hackrf_set_lna_gain");

        status = hackrf_set_vga_gain(device->device, 30);
        HACKRF_CHECK_STATUS(device, status, "hackrf_set_lna_gain");

        status = hackrf_start_rx(device->device, receive_sample_block, device);
        HACKRF_CHECK_STATUS(device, status, "hackrf_start_rx");

        memset(device->samples, 0, NRF_SAMPLES_SIZE * 3 * sizeof(float));
    } else {
        device->device = NULL;
        fprintf(stderr, "WARN nrf_start: Couldn't open HackRF. Falling back on data file %s\n", data_file);
        uint8_t int_buffer[NRF_SAMPLES_SIZE * 2];
        memset(int_buffer, 0, NRF_SAMPLES_SIZE * 2);
        if (data_file != NULL) {
            FILE *fp = fopen(data_file, "rb");
            if (fp != NULL) {
                fread(int_buffer, NRF_SAMPLES_SIZE * 2, 1, fp);
                fclose(fp);
            } else {
                fprintf(stderr, "WARN nrf_start: Couldn't open %s. Using empty buffer.\n", data_file);
            }
        }
        for (int i = 0; i < NRF_SAMPLES_SIZE * 2; i += 2) {
            float t = i / (float) NRF_SAMPLES_SIZE * 2;
            int vi = int_buffer[i];
            int vq = int_buffer[i + 1];
            vi = (vi + 128) % 256;
            vq = (vq + 128) % 256;
            _nrf_buffer_write(&device->buffer, vec3_init(vi / 256.0f, vq / 256.0f, t));
        }
    }

    return device;
}

// Change the frequency to the given frequency, in MHz.
void nrf_freq_set(nrf_device *device, double freq_mhz) {
    if (device->device == NULL) return;
    int status = hackrf_set_freq(device->device, freq_mhz * 1e6);
    HACKRF_CHECK_STATUS(device, status, "hackrf_set_freq");
}

void nrf_samples_consume(nrf_device *device, int sample_count) {
    for (int i = 0; i < sample_count; i++) {
        device->samples[i] = _nrf_buffer_read(&device->buffer);
    }
}

// Stop receiving data
void nrf_stop(nrf_device *device) {
    if (device->device != NULL) {
        hackrf_stop_rx(device->device);
        hackrf_close(device->device);
    }
    hackrf_exit();
    _nrf_buffer_destroy(&device->buffer);
    free(device);
}
