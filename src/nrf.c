// NDBX Radio Frequency functions, based on HackRF

#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include <libhackrf/hackrf.h>
#include <rtl-sdr.h>

#include "nrf.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
const double TAU = M_PI * 2;

// Block

void nrf_block_init(nrf_block* block, nrf_block_type type, nrf_block_process_fn process_fn, nrf_block_result_fn result_fn) {
    block->type = type;
    block->process_fn = process_fn;
    block->result_fn = result_fn;
    assert(block->n_outputs == 0);
}

void nrf_block_connect(nrf_block* input, nrf_block* output) {
    assert(input->n_outputs < NRF_BLOCK_MAX_OUTPUTS);
    input->outputs[input->n_outputs] = output;
    input->n_outputs++;
}

void nrf_block_process(nrf_block* block, nul_buffer* buffer) {
    if (block->process_fn != NULL) {
        block->process_fn(block, buffer);
    }

    if (block->n_outputs > 0) {
        nul_buffer *result = block->result_fn(block);
        for (int i = 0; i < block->n_outputs; i++) {
            nrf_block *output = block->outputs[i];
            nrf_block_process(output, result);
        }
        nul_buffer_free(result);
    }
}

// Device

void _nrf_rtlsdr_check_status(nrf_device *device, int status, const char *message, const char *file, int line) {
    if (status != 0) {
        fprintf(stderr, "RTL-SDR: %s (Status code %d) %s:%d\n", message, status, file, line);
        if (device == NULL && device->device != NULL) {

            rtlsdr_close((rtlsdr_dev_t*) device->device);
        }
        exit(EXIT_FAILURE);
    }
}

#define _NRF_RTLSDR_CHECK_STATUS(device, status, message) _nrf_rtlsdr_check_status(device, status, message, __FILE__, __LINE__)

void _nrf_hackrf_check_status(nrf_device *device, int status, const char *message, const char *file, int line) {
    if (status != 0) {
        fprintf(stderr, "NRF HackRF fatal error: %s\n", message);
        if (device != NULL && device->device != NULL) {
            hackrf_close((hackrf_device*) device->device);
        }
        hackrf_exit();
        exit(EXIT_FAILURE);
    }
}

#define _NRF_HACKRF_CHECK_STATUS(device, status, message) _nrf_hackrf_check_status(device, status, message, __FILE__, __LINE__)

static float _nrf_clampf(float v, float min, float max) {
    return v < min ? min : v > max ? max : v;
}

// Limit the frequency range to a possible value.
double _nrf_clamp_frequency(nrf_device *device, double freq_mhz) {
    if (device->device_type == NRF_DEVICE_RTLSDR) {
        return freq_mhz < 10 ? 10 : freq_mhz > 1766 ? 1766 : freq_mhz;
    } else if (device->device_type == NRF_DEVICE_HACKRF) {
        return freq_mhz < 1 ? 1 : freq_mhz > 6000 ? 6000 : freq_mhz;
    } else {
        return freq_mhz;
    }
}

static int _nrf_process_sample_block(nrf_device *device, uint8_t *buffer, int length) {
    assert(length == NRF_BUFFER_SIZE_BYTES);
    if (device->receiving == 0) return 0;

    pthread_mutex_lock(&device->data_mutex);
    for (int i = 0; i < length; i += 2) {
        uint8_t u8i = buffer[i];
        uint8_t u8q = buffer[i + 1];
        if (device->device_type == NRF_DEVICE_HACKRF || device->device_type == NRF_DEVICE_DUMMY) {
            u8i = (u8i + 128) % 256;
            u8q = (u8q + 128) % 256;
        }
        device->samples[i] = u8i;
        device->samples[i + 1] = u8q;
    }
    pthread_mutex_unlock(&device->data_mutex);

    if (device->decode_cb_fn != NULL) {
        device->decode_cb_fn(device, device->decode_cb_ctx);
    }

    if (device->receiving == 0) return 0;

    nrf_block_process(&device->block, NULL);

    // if (device->block.n_outputs > 0) {
    //     nul_buffer *buffer = nrf_device_get_samples_buffer(device);
    //     for (int i = 0; i < device->block.n_outputs; i++) {
    //         if (device->receiving == 0) return 0;
    //         nrf_block *output = device->block.outputs[i];
    //         nrf_block_process(output, buffer);
    //     }
    //     nul_buffer_free(buffer);
    // }

    return 0;
}

// This function will block, so needs to be called on its own thread.
void *_nrf_rtlsdr_receive_loop(nrf_device *device) {
    while (device->receiving) {
        int n_read;
        int status = rtlsdr_read_sync((rtlsdr_dev_t*) device->device, device->receive_buffer, NRF_BUFFER_SIZE_BYTES, &n_read);
        _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_read_sync");
        if (n_read < NRF_BUFFER_SIZE_BYTES) {
            fprintf(stderr, "Short read, samples lost, exiting!\n");
            exit(EXIT_FAILURE);
        }
        _nrf_process_sample_block(device, device->receive_buffer, NRF_BUFFER_SIZE_BYTES);
    }
    return NULL;
}

static int _nrf_hackrf_receive_sample_block(hackrf_transfer *transfer) {
    nrf_device *device = (nrf_device *)transfer->rx_ctx;
    return _nrf_process_sample_block(device, transfer->buffer, transfer->valid_length);
}

#define MILLISECS 1000000

static void _nrf_sleep_milliseconds(int millis) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = millis * MILLISECS;
    nanosleep(&ts, NULL);
}

static void _nrf_advance_block(nrf_device *device) {
    if (!device->paused) {
        device->dummy_block_index++;
        if (device->dummy_block_index >= device->dummy_block_length) {
            device->dummy_block_index = 0;
        }
    }
}

static void *_nrf_dummy_receive_loop(nrf_device *device) {
    while (device->receiving) {
        unsigned char *buffer = device->receive_buffer + (device->dummy_block_index * NRF_BUFFER_SIZE_BYTES);
        _nrf_process_sample_block(device, buffer, NRF_BUFFER_SIZE_BYTES);
        _nrf_advance_block(device);
        _nrf_sleep_milliseconds(1000 / 60);
    }
    return NULL;
}

static const int RTLSDR_DEFAULT_SAMPLE_RATE = 3e6;

static int _nrf_rtlsdr_start(nrf_device *device, double freq_mhz, int sample_rate) {
    int status;

    status = rtlsdr_open((rtlsdr_dev_t**)&device->device, 0);
    if (status != 0) {
        return status;
    }

    device->device_type = NRF_DEVICE_RTLSDR;
    device->receive_buffer = calloc(NRF_BUFFER_SIZE_BYTES, sizeof(uint8_t));

    rtlsdr_dev_t* dev = (rtlsdr_dev_t*) device->device;

    sample_rate = sample_rate != 0 ? sample_rate : RTLSDR_DEFAULT_SAMPLE_RATE;
    status = rtlsdr_set_sample_rate(dev, sample_rate);
    _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_set_sample_rate");
    device->sample_rate = sample_rate;

    // Set auto-gain mode
    status = rtlsdr_set_tuner_gain_mode(dev, 0);
    _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_set_tuner_gain_mode");

    status = rtlsdr_set_agc_mode(dev, 1);
    _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_set_agc_mode");

    freq_mhz = _nrf_clamp_frequency(device, freq_mhz);
    status = rtlsdr_set_center_freq(dev, freq_mhz * 1e6);
    _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_set_center_freq");

    status = rtlsdr_reset_buffer(dev);
    _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_reset_buffer");

    device->receiving = 1;
    pthread_create(&device->receive_thread, NULL, (void *(*)(void *)) &_nrf_rtlsdr_receive_loop, device);

    return 0;
}

static const int HACKRF_DEFAULT_SAMPLE_RATE = 5e6;

static int _nrf_hackrf_start(nrf_device *device, double freq_mhz, int sample_rate) {
    int status;

    status = hackrf_init();
    _NRF_HACKRF_CHECK_STATUS(NULL, status, "hackrf_init");

    status = hackrf_open((hackrf_device**)&device->device);
    if (status != 0) {
        return status;
    }

    device->device_type = NRF_DEVICE_HACKRF;

    hackrf_device *dev = (hackrf_device*) device->device;

    freq_mhz = _nrf_clamp_frequency(device, freq_mhz);
    status = hackrf_set_freq(dev, freq_mhz * 1e6);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_freq");

    sample_rate = sample_rate != 0 ? sample_rate : HACKRF_DEFAULT_SAMPLE_RATE;
    status = hackrf_set_sample_rate(dev, sample_rate);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_sample_rate");
    device->sample_rate = sample_rate;

    status = hackrf_set_amp_enable(dev, 0);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_amp_enable");

    status = hackrf_set_lna_gain(dev, 32);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_lna_gain");

    status = hackrf_set_vga_gain(dev, 40);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_lna_gain");

    device->receiving = 1;
    status = hackrf_start_rx(dev, _nrf_hackrf_receive_sample_block, device);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_start_rx");

    return 0;
}

static const int DUMMY_DEFAULT_SAMPLE_RATE = 5e6;

static int _nrf_dummy_start(nrf_device *device, const char *data_file) {
    device->device_type = NRF_DEVICE_DUMMY;
    device->sample_rate = DUMMY_DEFAULT_SAMPLE_RATE;

    fprintf(stderr, "WARN nrf_device_new: Couldn't open SDR device. Falling back on data file %s\n", data_file);
    if (data_file != NULL) {
        FILE *fp = fopen(data_file, "rb");
        if (fp != NULL) {
            fseek(fp, 0L, SEEK_END);
            long size = ftell(fp);
            rewind(fp);
            device->receive_buffer = calloc(size, sizeof(uint8_t));
            device->dummy_block_length = size / NRF_BUFFER_SIZE_BYTES;
            device->dummy_block_index = 0;
            fread(device->receive_buffer, size, 1, fp);
            fclose(fp);
        } else {
            fprintf(stderr, "WARN nrf_device_new: Couldn't open %s. Using empty buffer.\n", data_file);
            device->receive_buffer = calloc(NRF_BUFFER_SIZE_BYTES, sizeof(uint8_t));
            device->dummy_block_length = 1;
            device->dummy_block_index = 0;
        }
    }

    device->receiving = 1;
    pthread_create(&device->receive_thread, NULL, (void *(*)(void *))&_nrf_dummy_receive_loop, device);

    return 0;
}

// Start receiving on the given frequency.
// If the device could not be opened, use the raw contents of the data_file
// instead.
nrf_device *nrf_device_new(double freq_mhz, const char* data_file) {
    nrf_device_config config;
    memset(&config, 0, sizeof(nrf_device_config));
    config.freq_mhz = freq_mhz;
    config.data_file = data_file;
    return nrf_device_new_with_config(config);
}

nrf_device *nrf_device_new_with_config(const nrf_device_config config) {
    int sample_rate = config.sample_rate;
    double freq_mhz = config.freq_mhz > 0.1 ? config.freq_mhz : 100;
    const char *data_file = config.data_file != 0 ? config.data_file : NULL;

    int status;
    nrf_device *device = calloc(1, sizeof(nrf_device));
    nrf_block_init(&device->block, NRF_BLOCK_SOURCE, NULL, (nrf_block_result_fn) nrf_device_get_samples_buffer);
    pthread_mutex_init(&device->data_mutex, NULL);
    memset(device->samples, 0, NRF_BUFFER_SIZE_BYTES);

    // Try to find a suitable hardware device, fall back to data file.
    status = _nrf_rtlsdr_start(device, freq_mhz, sample_rate);
    if (status != 0) {
        status = _nrf_hackrf_start(device, freq_mhz, sample_rate);
        if (status != 0) {
            status = _nrf_dummy_start(device, data_file);
            if (status != 0) {
                fprintf(stderr, "ERROR nrf_device_new: Couldn't even start dummy device. Exiting.\n");
            }
        }
    }

    return device;
}

// Change the frequency to the given frequency, in MHz.
double nrf_device_set_frequency(nrf_device *device, double freq_mhz) {
    freq_mhz = _nrf_clamp_frequency(device, freq_mhz);
    if (device->device_type == NRF_DEVICE_RTLSDR) {
        int status = rtlsdr_set_center_freq((rtlsdr_dev_t*) device->device, freq_mhz * 1e6);
        _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_set_center_freq");
    } else if (device->device_type == NRF_DEVICE_HACKRF) {
        int status = hackrf_set_freq(device->device, freq_mhz * 1e6);
        _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_freq");
    }
    return freq_mhz;
}

void nrf_device_set_decode_handler(nrf_device *device, nrf_device_decode_cb_fn fn, void *ctx) {
    device->decode_cb_fn = fn;
    device->decode_cb_ctx = ctx;
}

void nrf_device_set_paused(nrf_device *device, int paused) {
    device->paused = paused;
}

void nrf_device_step(nrf_device *device) {
    device->dummy_block_index++;
    if (device->dummy_block_index >= device->dummy_block_length) {
        device->dummy_block_index = 0;
    }
}

nul_buffer *nrf_device_get_samples_buffer(nrf_device *device) {
    pthread_mutex_lock(&device->data_mutex);
    nul_buffer *buffer = nul_buffer_new_u8(NRF_SAMPLES_LENGTH, 2, device->samples);
    pthread_mutex_unlock(&device->data_mutex);
    return buffer;
}

nul_buffer *nrf_device_get_iq_buffer(nrf_device *device) {
    pthread_mutex_lock(&device->data_mutex);
    nul_buffer *buffer = nul_buffer_new_u8(NRF_IQ_RESOLUTION * NRF_IQ_RESOLUTION, 1, NULL);
    for (int i = 0; i < NRF_BUFFER_SIZE_BYTES; i += 2) {
        int u8i = device->samples[i];
        int u8q = device->samples[i + 1];
        int offset = u8i * 256 + u8q;
        buffer->data.u8[offset]++;
    }
    pthread_mutex_unlock(&device->data_mutex);
    return buffer;
}

static void pixel_inc(nul_buffer *image_buffer, int stride, int x, int y) {
    int offset = y * stride + x;
    if (image_buffer->type == NUL_BUFFER_U8) {
        image_buffer->data.u8[offset]++;
    } else {
        image_buffer->data.f64[offset]++;
    }
}

static void draw_line(nul_buffer *image_buffer, int stride, int x1, int y1, int x2, int y2, int color) {
  int dx = abs(x2 - x1);
  int sx = x1 < x2 ? 1 : -1;
  int dy = abs(y2-y1);
  int sy = y1 < y2 ? 1 : -1;
  int err = (dx > dy ? dx : -dy) / 2;
  int e2;

  for(;;){
    pixel_inc(image_buffer, stride, x1, y1);
    if (x1 == x2 && y1 == y2) break;
    e2 = err;
    if (e2 > -dx) { err -= dy; x1 += sx; }
    if (e2 <  dy) { err += dx; y1 += sy; }
  }
}

nul_buffer *nrf_device_get_iq_lines(nrf_device *device, int size_multiplier, float line_percentage) {
    line_percentage = _nrf_clampf(line_percentage, 0, 1);
    pthread_mutex_lock(&device->data_mutex);
    int sz = NRF_IQ_RESOLUTION * size_multiplier;
    nul_buffer *image_buffer = nul_buffer_new_u8(sz * sz, 1, NULL);
    int x1 = 0;
    int y1 = 0;
    int max = NRF_BUFFER_SIZE_BYTES * line_percentage;
    for (int i = 0; i < max; i += 2) {
        int x2 = device->samples[i] * size_multiplier;
        int y2 = device->samples[i + 1] * size_multiplier;
        if (i > 0) {
            draw_line(image_buffer, NRF_IQ_RESOLUTION * size_multiplier, x1, y1, x2, y2, 0);
        }
        x1 = x2;
        y1 = y2;
    }
    pthread_mutex_unlock(&device->data_mutex);
    return image_buffer;
}

// Stop receiving data
void nrf_device_free(nrf_device *device) {
    if (device->device_type == NRF_DEVICE_RTLSDR) {
        device->receiving = 0;
        pthread_join(device->receive_thread, NULL);
        rtlsdr_close((rtlsdr_dev_t*) device->device);
    } else if (device->device_type == NRF_DEVICE_HACKRF) {
        hackrf_stop_rx((hackrf_device*) device->device);
        hackrf_close((hackrf_device*) device->device);
        hackrf_exit();
    } else if (device->device_type == NRF_DEVICE_DUMMY) {
        device->receiving = 0;
        pthread_join(device->receive_thread, NULL);
    }
    if (device->receive_buffer) {
        free(device->receive_buffer);
    }
    free(device);
}

// Interpolator

nrf_interpolator *nrf_interpolator_new(double interpolate_step) {
    nrf_interpolator *interpolator = calloc(1, sizeof(nrf_interpolator));
    interpolator->interpolate_step = interpolate_step;
    interpolator->t = -1;
    return interpolator;
}

void nrf_interpolator_process(nrf_interpolator *interpolator, nul_buffer *buffer) {
    if (interpolator->t < 0.0) {
        // Special start-up condition. Set b_buffer and interpolate from zero.
        if (buffer->type == NUL_BUFFER_U8) {
            interpolator->buffer_a = nul_buffer_new_u8(buffer->length, buffer->channels, NULL);
        } else {
            interpolator->buffer_a = nul_buffer_new_f64(buffer->length, buffer->channels, NULL);
        }
        interpolator->buffer_b = nul_buffer_copy(buffer);
        interpolator->t = 0.0;
    } else if (interpolator->t >= 1.0) {
        nul_buffer_set_data(interpolator->buffer_a, interpolator->buffer_b);
        nul_buffer_set_data(interpolator->buffer_b, buffer);
        interpolator->t = 0.0;
    } else

    interpolator->t += interpolator->interpolate_step;
}

nul_buffer *nrf_interpolator_get_buffer(nrf_interpolator *interpolator) {
    nul_buffer *a = interpolator->buffer_a;
    nul_buffer *b = interpolator->buffer_b;
    double t = interpolator->t;
    assert(a->type == b->type);
    assert(a->size_bytes == b->size_bytes);
    nul_buffer *dst;
    if (a->type == NUL_BUFFER_U8) {
        dst = nul_buffer_new_u8(a->length, a->channels, NULL);
    } else {
        dst = nul_buffer_new_f64(a->length, a->channels, NULL);
    }
    int size = a->length * a->channels;
    for (int i = 0; i < size; i++) {
        double va = nul_buffer_get_f64(a, i);
        double vb = nul_buffer_get_f64(b, i);
        double v = va * (1.0 - t) + vb * t;
        nul_buffer_set_f64(dst, i, v);
    }
    return dst;
}

void nrf_interpolator_free(nrf_interpolator *interpolator) {
    nul_buffer_free(interpolator->buffer_a);
    nul_buffer_free(interpolator->buffer_b);
    free(interpolator);
}

// IQ Drawing

// Convert a buffer with raw samples to a buffer with I/Q points.
nul_buffer *nrf_buffer_to_iq_points(nul_buffer *buffer) {
    nul_buffer *img = nul_buffer_new_u8(NRF_IQ_RESOLUTION * NRF_IQ_RESOLUTION, 1, NULL);
    int size = buffer->length * buffer->channels;
    for (int i = 0; i < size; i += 2) {
        int u8i = nul_buffer_get_u8(buffer, i);
        int u8q = nul_buffer_get_u8(buffer, i + 1);
        int offset = u8i * NRF_IQ_RESOLUTION + u8q;
        img->data.u8[offset]++;
    }
    return img;
}

// Convert a buffer to I/Q lines.
nul_buffer *nrf_buffer_to_iq_lines(nul_buffer *buffer, int size_multiplier, float line_percentage) {
    line_percentage = _nrf_clampf(line_percentage, 0, 1);
    int sz = NRF_IQ_RESOLUTION * size_multiplier;
    nul_buffer *image_buffer = nul_buffer_new_u8(sz * sz, 1, NULL);
    int x1 = 0;
    int y1 = 0;
    int size = buffer->length * buffer->channels;
    int max = size * line_percentage;
    for (int i = 0; i < max; i += 2) {
        int x2 = nul_buffer_get_u8(buffer, i) * size_multiplier;
        int y2 = nul_buffer_get_u8(buffer, i + 1) * size_multiplier;
        if (i > 0) {
            draw_line(image_buffer, NRF_IQ_RESOLUTION * size_multiplier, x1, y1, x2, y2, 0);
        }
        x1 = x2;
        y1 = y2;
    }
    return image_buffer;
}

// FFT Analysis

nrf_fft *nrf_fft_new(int fft_size, int fft_history_size) {
    nrf_fft *fft = calloc(1, sizeof(nrf_fft));
    nrf_block_init(&fft->block, NRF_BLOCK_GENERIC, (nrf_block_process_fn) nrf_fft_process, (nrf_block_result_fn) nrf_fft_get_buffer);
    fft->fft_size = fft_size;
    fft->fft_history_size = fft_history_size;
    fft->fft_in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NRF_SAMPLES_LENGTH);
    fft->fft_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NRF_SAMPLES_LENGTH);
    fft->fft_plan = fftw_plan_dft_1d(fft_size, fft->fft_in, fft->fft_out, FFTW_FORWARD, FFTW_MEASURE);
    fft->buffer = calloc(fft_size * fft_history_size * 2, sizeof(double));
    return fft;
}

void nrf_fft_process(nrf_fft *fft, nul_buffer *buffer) {
    int size = buffer->length * buffer->channels;
    assert(size == NRF_BUFFER_SIZE_BYTES);
    int ii = 0;
    for (int i = 0; i < size; i += 2) {
        fftw_complex *p = fft->fft_in;
        double di, dq;
        if (buffer->type == NUL_BUFFER_U8) {
            di = buffer->data.u8[i] / 256.0;
            dq = buffer->data.u8[i + 1] / 256.0;
        } else {
            di = buffer->data.f64[i];
            dq = buffer->data.f64[i + 1];
        }
        p[ii][0] = powf(-1, ii) * di;
        p[ii][1] = powf(-1, ii) * dq;
        ii++;
    }
    fftw_execute(fft->fft_plan);
    // Move the previous lines down
    memcpy(fft->buffer + fft->fft_size * 2, fft->buffer, fft->fft_size * (fft->fft_history_size - 1) * 2 * sizeof(double));
    // Set the first line
    int j = 0;
    for (int i = 0; i < fft->fft_size; i++) {
        fftw_complex *out = fft->fft_out;
        fft->buffer[j++] = out[i][0];
        fft->buffer[j++] = out[i][1];
    }
}

nul_buffer *nrf_fft_get_buffer(nrf_fft *fft) {
    return nul_buffer_new_f64(fft->fft_size * fft->fft_history_size, 2, (double *) fft->buffer);
}

void nrf_fft_free(nrf_fft *fft) {
    fftw_destroy_plan(fft->fft_plan);
    fftw_free(fft->fft_in);
    fftw_free(fft->fft_out);
    free(fft);
}

// Finite Impulse Response (FIR) Filter

// Generates coefficients for a FIR low-pass filter with the given
// half-amplitude frequency and kernel length at the given sample rate.
//
// sample_rate    - The signal's sample rate.
// half_ampl_freq - The half-amplitude frequency in Hz.
// length         - The filter kernel's length. Should be an odd number.
//
// Returns the FIR coefficients for the filter.
double *nrf_fir_get_low_pass_coefficients(int sample_rate, int half_ampl_freq, int length) {
    length += (length + 1) % 2;
    double freq = half_ampl_freq / (double) sample_rate;
    double *coefs = calloc(length, sizeof(double));
    int center = floor(length / 2);
    double sum = 0;
    for (int i = 0; i < length; i++) {
        double val;
        if (i == center) {
            val = TAU * freq;
        } else {
            double angle = TAU * (i + 1) / (double) (length + 1);
            val = sin(TAU * freq * (i - center)) / (double) (i - center);
            val *= 0.42 - 0.5 * cos(angle) + 0.08 * cos(2 * angle);
        }
        sum += val;
        coefs[i] = val;
    }
    for (int i = 0; i < length; i++) {
        coefs[i] /= sum;
    }
    return coefs;
}

nrf_fir_filter *nrf_fir_filter_new(int sample_rate, int half_ampl_freq, int length) {
    nrf_fir_filter *filter = calloc(1, sizeof(nrf_fir_filter));
    filter->length = length;
    filter->coefficients = nrf_fir_get_low_pass_coefficients(sample_rate, half_ampl_freq, length);
    filter->offset = length - 1;
    filter->center = floor(length / 2);
    filter->samples_length = filter->offset;
    filter->samples = calloc(filter->samples_length, sizeof(double));
    return filter;
}

void nrf_fir_filter_load(nrf_fir_filter *filter, double *samples, int length) {
    int should_free = 0;
    int new_length = length + filter->offset;
    double *new_samples;
    if (filter->samples_length != new_length) {
        if (filter->samples_length != 0) {
            should_free = 1;
        }
        new_samples = calloc(new_length, sizeof(double));
    } else {
        new_samples = filter->samples;
    }

    // Copy the last `offset` samples to the new buffer.
    memcpy(new_samples, filter->samples + filter->samples_length - filter->offset, filter->offset * sizeof(double));
    memcpy(new_samples + filter->offset, samples, length * sizeof(double));

    if (should_free) {
        free(filter->samples);
    }
    filter->samples = new_samples;
    filter->samples_length = new_length;
}

double nrf_fir_filter_get(nrf_fir_filter *filter, int index) {
    double v = 0;
    for (int i = 0; i < filter->length; i++) {
        v += filter->coefficients[i] * filter->samples[index + i];
    }
    return v;
}

void nrf_fir_filter_free(nrf_fir_filter *filter) {
    filter->samples_length = 0;
    free(filter->coefficients);
    free(filter->samples);
    free(filter);
}

// IQ Filter

nrf_iq_filter *nrf_iq_filter_new(int sample_rate, int half_ampl_freq, int kernel_length) {
    nrf_iq_filter *f = calloc(1, sizeof(nrf_iq_filter));
    nrf_block_init(&f->block, NRF_BLOCK_GENERIC, (nrf_block_process_fn) nrf_iq_filter_process, (nrf_block_result_fn) nrf_iq_filter_get_buffer);
    f->filter_i = nrf_fir_filter_new(sample_rate, half_ampl_freq, kernel_length);
    f->filter_q = nrf_fir_filter_new(sample_rate, half_ampl_freq, kernel_length);
    return f;
}

void nrf_iq_filter_process(nrf_iq_filter *filter, nul_buffer *buffer) {
    int old_length = filter->samples_length;
    int length = buffer->length;
    if (length != old_length) {
        free(filter->samples_i);
        free(filter->samples_q);
        filter->samples_i = calloc(length, sizeof(double));
        filter->samples_q = calloc(length, sizeof(double));
    }
    filter->samples_length = length;

    int j = 0;
    for (int i = 0; i < length * 2; i += 2) {
        filter->samples_i[j] = nul_buffer_get_f64(buffer, i);
        filter->samples_q[j] = nul_buffer_get_f64(buffer, i + 1);
        j++;
    }
    nrf_fir_filter_load(filter->filter_i, filter->samples_i, length);
    nrf_fir_filter_load(filter->filter_q, filter->samples_q, length);
}

nul_buffer *nrf_iq_filter_get_buffer(nrf_iq_filter *f) {
    int length = f->samples_length;
    nul_buffer *result = nul_buffer_new_f64(length, 2, NULL);
    int k = 0;
    for (int i = 0; i < length; i++) {
        result->data.f64[k++] = nrf_fir_filter_get(f->filter_i, i);
        result->data.f64[k++] = nrf_fir_filter_get(f->filter_q, i);
    }
    return result;
}

void nrf_iq_filter_free(nrf_iq_filter *f) {
    nrf_fir_filter_free(f->filter_i);
    nrf_fir_filter_free(f->filter_q);
    free(f->samples_i);
    free(f->samples_q);
    free(f);
}

// Downsampler

nrf_downsampler *nrf_downsampler_new(int in_rate, int out_rate, int filter_freq, int kernel_length) {
    nrf_downsampler *d = calloc(1, sizeof(nrf_downsampler));
    d->in_rate = in_rate;
    d->out_rate = out_rate;
    d->filter = nrf_fir_filter_new(in_rate, filter_freq, kernel_length);
    d->rate_mul = in_rate / (double) out_rate;
    d->out_length = 0;
    d->out_samples = NULL;
    return d;
}

void nrf_downsampler_process(nrf_downsampler *d, double *samples, int length) {
    nrf_fir_filter_load(d->filter, samples, length);

    // FIXME: Optimize by comparing out_length to d->out_length
    free(d->out_samples);
    d->out_length = floor(length / d->rate_mul);
    d->out_samples = calloc(d->out_length, sizeof(double));

    double t = 0;
    for (int i = 0; i < d->out_length; i++) {
        d->out_samples[i] = nrf_fir_filter_get(d->filter, floor(t));
        t += d->rate_mul;
    }
}

void nrf_downsampler_free(nrf_downsampler *d) {
    nrf_fir_filter_free(d->filter);
    free(d->out_samples);
    free(d);
}

// Frequency shifter

nrf_freq_shifter *nrf_freq_shifter_new(int freq_offset, int sample_rate) {
    nrf_freq_shifter *shifter = calloc(1, sizeof(nrf_freq_shifter));
    nrf_block_init(&shifter->block, NRF_BLOCK_GENERIC, (nrf_block_process_fn) nrf_freq_shifter_process, (nrf_block_result_fn) nrf_freq_shifter_get_buffer);
    shifter->freq_offset = freq_offset;
    shifter->sample_rate = sample_rate;
    shifter->cosine = 1;
    shifter->sine = 0;
    return shifter;
}

void nrf_freq_shifter_process_samples(nrf_freq_shifter *shifter, double *samples_i, double *samples_q, int length) {
    double delta_cos = cos(TAU * shifter->freq_offset / (double) shifter->sample_rate);
    double delta_sin = sin(TAU * shifter->freq_offset / (double) shifter->sample_rate);
    double cosine = shifter->cosine;
    double sine = shifter->sine;
    for (int i = 0; i < length; i++) {
        double vi = samples_i[i];
        double vq = samples_q[i];
        samples_i[i] = vi * cosine - vq * sine;
        samples_q[i] = vi * sine + vq * cosine;
        double new_sine = cosine * delta_sin + sine * delta_cos;
        double new_cosine = cosine * delta_cos - sine * delta_sin;
        sine = new_sine;
        cosine = new_cosine;
    }
    shifter->cosine = cosine;
    shifter->sine = sine;
}

void nrf_freq_shifter_process(nrf_freq_shifter *shifter, nul_buffer *buffer) {
    double delta_cos = cos(TAU * shifter->freq_offset / (double) shifter->sample_rate);
    double delta_sin = sin(TAU * shifter->freq_offset / (double) shifter->sample_rate);
    double cosine = shifter->cosine;
    double sine = shifter->sine;
    assert(buffer->channels == 2);
    int size = buffer->length * buffer->channels;
    if (shifter->buffer == NULL) {
        shifter->buffer = nul_buffer_new_f64(size, 2, NULL);
    }
    double *out_samples = shifter->buffer->data.f64;
    for (int i = 0; i < size; i += 2) {
        double vi = nul_buffer_get_f64(buffer, i);
        double vq = nul_buffer_get_f64(buffer, i + 1);
        out_samples[i] = vi * cosine - vq * sine;
        out_samples[i + 1] = vi * sine + vq * cosine;
        double new_sine = cosine * delta_sin + sine * delta_cos;
        double new_cosine = cosine * delta_cos - sine * delta_sin;
        sine = new_sine;
        cosine = new_cosine;
    }
    shifter->cosine = cosine;
    shifter->sine = sine;
}

nul_buffer *nrf_freq_shifter_get_buffer(nrf_freq_shifter *shifter) {
    return nul_buffer_copy(shifter->buffer);
}

void nrf_freq_shifter_free(nrf_freq_shifter *shifter) {
    free(shifter);
}

// RAW Demodulator

nrf_raw_demodulator *nrf_raw_demodulator_new(int in_sample_rate, int out_sample_rate) {
    nrf_raw_demodulator *d = calloc(1, sizeof(nrf_raw_demodulator));
    d->in_sample_rate = in_sample_rate;
    d->out_sample_rate = out_sample_rate;
    d->downsampler_audio = nrf_downsampler_new(in_sample_rate, out_sample_rate, out_sample_rate / 2, 41);
    return d;
}

void nrf_raw_demodulator_process(nrf_raw_demodulator *demodulator, double *samples_i, double *samples_q, int length) {
    nrf_downsampler_process(demodulator->downsampler_audio, samples_i, length);

    // Copy audio samples to demodulator
    int audio_samples_length = demodulator->downsampler_audio->out_length;
    if (audio_samples_length != demodulator->audio_samples_length) {
        free(demodulator->audio_samples);
        demodulator->audio_samples = calloc(audio_samples_length, sizeof(double));
        demodulator->audio_samples_length = audio_samples_length;
    }
    memcpy(demodulator->audio_samples, demodulator->downsampler_audio->out_samples, audio_samples_length * sizeof(double));
}

void nrf_raw_demodulator_free(nrf_raw_demodulator *demodulator) {
    nrf_downsampler_free(demodulator->downsampler_audio);
    free(demodulator->audio_samples);
    free(demodulator);
}

// FM Demodulator

nrf_fm_demodulator *nrf_fm_demodulator_new(int in_sample_rate, int out_sample_rate) {
    nrf_fm_demodulator *d = calloc(1, sizeof(nrf_fm_demodulator));
    const int inter_rate = 336000;
    const int  max_f = 75000;
    const double filter_freq = max_f * 0.8;
    d->in_sample_rate = in_sample_rate;
    d->out_sample_rate = out_sample_rate;
    d->ampl_conv = out_sample_rate / (TAU * max_f);
    d->downsampler_i = nrf_downsampler_new(in_sample_rate, inter_rate, filter_freq, 51);
    d->downsampler_q = nrf_downsampler_new(in_sample_rate, inter_rate, filter_freq, 51);
    d->downsampler_audio = nrf_downsampler_new(inter_rate, out_sample_rate, 10000, 41);
    return d;
}

void nrf_fm_demodulator_process(nrf_fm_demodulator *demodulator, double *samples_i, double *samples_q, int length) {
    // Downsample
    nrf_downsampler_process(demodulator->downsampler_i, samples_i, length);
    nrf_downsampler_process(demodulator->downsampler_q, samples_q, length);

    // Allocate demodulated samples buffer
    int demodulated_length = demodulator->downsampler_i->out_length;
    if (demodulated_length != demodulator->demodulated_length) {
        free(demodulator->demodulated_samples);
        demodulator->demodulated_samples = calloc(demodulated_length, sizeof(double));
        demodulator->demodulated_length = demodulated_length;
    }
    double *demodulated_samples = demodulator->demodulated_samples;

    // Actual FM demodulation
    double prev = 0;
    double delta_sum_squared = 0;
    double l_i = demodulator->l_i;
    double l_q = demodulator->l_q;
    for (int i = 0; i < demodulated_length; i++) {
        double real = l_i * demodulator->downsampler_i->out_samples[i] + l_q * demodulator->downsampler_q->out_samples[i];
        double imag = l_i * demodulator->downsampler_q->out_samples[i] - demodulator->downsampler_i->out_samples[i] * l_q;
        double sgn = 1;
        if (imag < 0) {
            sgn *= -1;
            imag *= -1;
        }
        double ang = 0;
        double div;
        if (real == imag) {
            div = 1;
        } else if (real > imag) {
            div = imag / real;
        } else {
            ang = -M_PI / 2;
            div = real / imag;
            sgn *= -1;
        }
        demodulated_samples[i] = sgn *
            (ang + div
                / (0.98419158358617365
                    + div * (0.093485702629671305
                        + div * 0.19556307900617517))) * demodulator->ampl_conv;
        l_i = demodulator->downsampler_i->out_samples[i];
        l_q = demodulator->downsampler_q->out_samples[i];
        double delta = prev - demodulated_samples[i];
        delta_sum_squared += delta * delta;
        prev = demodulated_samples[i];
    }
    demodulator->l_i = l_i;
    demodulator->l_q = l_q;

    // Downsample again, for audio
    nrf_downsampler_process(demodulator->downsampler_audio, demodulated_samples, demodulated_length);

   // Copy audio samples to demodulator
    int audio_samples_length = demodulator->downsampler_audio->out_length;
    if (audio_samples_length != demodulator->audio_samples_length) {
        free(demodulator->audio_samples);
        demodulator->audio_samples = calloc(audio_samples_length, sizeof(double));
        demodulator->audio_samples_length = audio_samples_length;
    }
    memcpy(demodulator->audio_samples, demodulator->downsampler_audio->out_samples, audio_samples_length * sizeof(double));
    double *audio_samples = demodulator->audio_samples;

    // De-emphasize samples
    double alpha = 1.0 / (1.0 + demodulator->out_sample_rate * 50.0 / 1e6);
    double val = demodulator->deemphasis_val;
    for (int i = 0; i < audio_samples_length; i++) {
        val = val + alpha * (audio_samples[i] - val);
        audio_samples[i] = val;
    }
    demodulator->deemphasis_val = val;
}

void nrf_fm_demodulator_free(nrf_fm_demodulator *demodulator) {
    nrf_downsampler_free(demodulator->downsampler_i);
    nrf_downsampler_free(demodulator->downsampler_q);
    nrf_downsampler_free(demodulator->downsampler_audio);
    free(demodulator->demodulated_samples);
    free(demodulator->audio_samples);
    free(demodulator);
}

// Decoder

nrf_decoder *nrf_decoder_new(nrf_demodulate_type demodulate_type, int in_sample_rate, int out_sample_rate, int freq_offset) {
    nrf_decoder *decoder = calloc(1, sizeof(nrf_decoder));
    decoder->in_sample_rate = in_sample_rate;
    decoder->out_sample_rate = out_sample_rate;
    decoder->demodulate_type = demodulate_type;
    if (decoder->demodulate_type == NRF_DEMODULATE_RAW) {
        decoder->demodulator = nrf_raw_demodulator_new(decoder->in_sample_rate, decoder->out_sample_rate);
    } else if (decoder->demodulate_type == NRF_DEMODULATE_WBFM) {
        decoder->demodulator = nrf_fm_demodulator_new(decoder->in_sample_rate, decoder->out_sample_rate);
    }
    decoder->freq_shifter = nrf_freq_shifter_new(freq_offset, in_sample_rate);
    return decoder;
}

void nrf_decoder_process(nrf_decoder *decoder, uint8_t *buffer, size_t length) {
    // Convert 8-bit samples to doubles
    if (decoder->samples_length != length) {
        free(decoder->samples_i);
        free(decoder->samples_q);
        decoder->samples_i = calloc(length, sizeof(double));
        decoder->samples_q = calloc(length, sizeof(double));
    }

    double *samples_i = decoder->samples_i;
    double *samples_q = decoder->samples_q;
    for (int i = 0; i < length; i++) {
        double vi = buffer[i * 2]  / 128.0 - 0.995;
        double vq = buffer[i * 2 + 1]  / 128.0 - 0.995;
        samples_i[i] = vi;
        samples_q[i] = vq;
    }

    // Shift frequency
    nrf_freq_shifter_process_samples(decoder->freq_shifter, samples_i, samples_q, length);

    // Demodulate
    if (decoder->demodulate_type == NRF_DEMODULATE_RAW) {
        nrf_raw_demodulator *demodulator = (nrf_raw_demodulator*) decoder->demodulator;
        nrf_raw_demodulator_process(demodulator, samples_i, samples_q, length);
        // FIXME: memcpy?
        decoder->audio_samples = demodulator->audio_samples;
        decoder->audio_samples_length = demodulator->audio_samples_length;
    } else if (decoder->demodulate_type == NRF_DEMODULATE_WBFM) {
        nrf_fm_demodulator *demodulator = (nrf_fm_demodulator*) decoder->demodulator;
        nrf_fm_demodulator_process(demodulator, samples_i, samples_q, length);
        // FIXME: memcpy?
        decoder->audio_samples = demodulator->audio_samples;
        decoder->audio_samples_length = demodulator->audio_samples_length;
    }
}

void nrf_decoder_free(nrf_decoder *decoder) {
    if (decoder->demodulate_type == NRF_DEMODULATE_RAW) {
        nrf_raw_demodulator_free(decoder->demodulator);
    } else if (decoder->demodulate_type == NRF_DEMODULATE_WBFM) {
        nrf_fm_demodulator_free(decoder->demodulator);
    }
    nrf_freq_shifter_free(decoder->freq_shifter);
    free(decoder);
}

// Buffer queue

static _nul_buffer_queue *_nul_buffer_queue_new(int capacity) {
    _nul_buffer_queue *q = calloc(1, sizeof(_nul_buffer_queue));
    q->size = 0;
    q->capacity = capacity;
    q->values = calloc(capacity, sizeof(ALuint));
    return q;
}

static void _nul_buffer_queue_push(_nul_buffer_queue *q, ALuint v) {
    if (q->size + 1 > q->capacity) {
        fprintf(stderr, "Queue is too small (capacity: %d)\n", q->capacity);
    }
    q->values[q->size] = v;
    q->size++;
}

static ALuint _nul_buffer_queue_pop(_nul_buffer_queue *q) {
    if (q->size == 0) {
        fprintf(stderr, "No more items to pop.\n");
    }
    int v = q->values[0];
    // Shift all items.
    for (int i = 1; i < q->size; i++) {
        q->values[i-1] = q->values[i];
    }
    q->size--;
    return v;
}

static void _nul_buffer_queue_free(_nul_buffer_queue *q) {
    free(q->values);
    free(q);
}

// Audio Player

static const int AUDIO_SAMPLE_RATE = 48000;
static const ALenum AL_BUFFER_FORMAT = AL_FORMAT_MONO16;

static void _nrf_al_check_error(const char *file, int line) {
    ALenum err = alGetError();
    int has_error = 0;
    while (err != AL_NO_ERROR) {
        has_error = 1;
        char *msg = NULL;
        switch (err) {
            case AL_INVALID_NAME:
                msg = "AL_INVALID_NAME";
                break;
            case AL_INVALID_ENUM:
                msg = "AL_INVALID_ENUM";
                break;
            case AL_INVALID_VALUE:
                msg = "AL_INVALID_VALUE";
                break;
            case AL_INVALID_OPERATION:
                msg = "AL_INVALID_OPERATION";
                break;
            case AL_OUT_OF_MEMORY:
                msg = "AL_OUT_OF_MEMORY";
                break;
        }
        fprintf(stderr, "OpenAL error: %s - %s:%d\n", msg, file, line);
        err = alGetError();
    }
    if (has_error) {
        exit(EXIT_FAILURE);
    }
}

#define _NRF_AL_CHECK_ERROR() _nrf_al_check_error(__FILE__, __LINE__)

void _nrf_player_decode(nrf_device *device, void *ctx) {
    nrf_player *player = (nrf_player *) ctx;

    if (player->shutting_down) return;

    // Decode/demodulate the signal.
    nrf_decoder_process(player->decoder, device->samples, NRF_SAMPLES_LENGTH);

    if (player->shutting_down) return;

    // Convert to signed 16-bit integers.
    double *audio_samples = player->decoder->audio_samples;
    int audio_samples_length = player->decoder->audio_samples_length;
    int16_t *pcm_samples = calloc(audio_samples_length, sizeof(int16_t));
    for (int i = 0; i < audio_samples_length; i++) {
        pcm_samples[i] = audio_samples[i] * 32000;
    }

    if (player->shutting_down) return;

    // Check if there are processed buffers we need to unqueue
    int processed_buffers;
    alGetSourceiv(player->audio_source, AL_BUFFERS_PROCESSED, &processed_buffers);
    _NRF_AL_CHECK_ERROR();
    assert (processed_buffers <= player->audio_buffer_queue->size);
    while (processed_buffers > 0) {
        ALuint buffer_id = _nul_buffer_queue_pop(player->audio_buffer_queue);
        alSourceUnqueueBuffers(player->audio_source, 1, &buffer_id);
        _NRF_AL_CHECK_ERROR();
        alDeleteBuffers(1, &buffer_id);
        processed_buffers--;
    }

    // Initialize an audio buffer
    ALuint buffer_id;
    alGenBuffers(1, &buffer_id);
    _NRF_AL_CHECK_ERROR();
    _nul_buffer_queue_push(player->audio_buffer_queue, buffer_id);

    // Set the data for the buffer
    alBufferData(buffer_id, AL_BUFFER_FORMAT, pcm_samples, audio_samples_length * sizeof(int16_t), AUDIO_SAMPLE_RATE);
    _NRF_AL_CHECK_ERROR();

    alSourceQueueBuffers(player->audio_source, 1, &buffer_id);
    _NRF_AL_CHECK_ERROR();

    if (!player->is_playing && player->audio_buffer_queue->size > 2) {
        alSourcePlay(player->audio_source);
        _NRF_AL_CHECK_ERROR();
        player->is_playing = 1;
    }

    // The data is now stored in OpenAL, delete our PCM sample buffer.
    free(pcm_samples);
}

nrf_player *nrf_player_new(nrf_device *device, nrf_demodulate_type demodulate_type, int freq_offset) {
    nrf_player *player = calloc(1, sizeof(nrf_player));
    player->device = device;
    player->decoder = nrf_decoder_new(demodulate_type, device->sample_rate, AUDIO_SAMPLE_RATE, freq_offset);

     // Initialize the audio context
    player->audio_device = alcOpenDevice(NULL);
    if (!device) {
        fprintf(stderr, "Could not open audio device.\n");
        exit(EXIT_FAILURE);
    }
    player->audio_context = alcCreateContext(player->audio_device, NULL);
    alcMakeContextCurrent(player->audio_context);
    _NRF_AL_CHECK_ERROR();

    // Initialize an audio source.
    alGenSources(1, &player->audio_source);
    _NRF_AL_CHECK_ERROR();

    // Turn off looping.
    alSourcei(player->audio_source, AL_LOOPING, AL_FALSE);
    _NRF_AL_CHECK_ERROR();

    // Create an audio buffer queue.
    player->audio_buffer_queue = _nul_buffer_queue_new(1000);

    // Register device callback
    nrf_device_set_decode_handler(device, _nrf_player_decode, player);

    return player;
}

void nrf_player_set_freq_offset(nrf_player *player, int freq_offset) {
    player->decoder->freq_shifter->freq_offset = freq_offset;
}

void nrf_player_free(nrf_player *player) {
    player->shutting_down = 1;
    nrf_device_set_decode_handler(player->device, NULL, NULL);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(player->audio_context);
    alcCloseDevice(player->audio_device);
    player->shutting_down = 1;

    // Note we don't own the NRF device, so we're not going to free it.
    nrf_decoder_free(player->decoder);
    _nul_buffer_queue_free(player->audio_buffer_queue);
    free(player);
}
