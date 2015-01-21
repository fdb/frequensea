// NDBX Radio Frequency functions, based on HackRF

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <libhackrf/hackrf.h>
#include <rtl-sdr.h>

#include "nrf.h"

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

static int _nrf_lerp(float a, float b, float t) {
    return a * (1.0 - t) + b * t;
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

static int _nrf_process_sample_block(nrf_device *device, unsigned char *buffer, int length) {
    assert(length == NRF_BUFFER_LENGTH);

    if (device->t >= 1.0) {
        memcpy(device->a_buffer, device->b_buffer, NRF_BUFFER_LENGTH);
        memcpy(device->b_buffer, buffer, NRF_BUFFER_LENGTH);
        device->t = 0.0;
    } else if (device->t < 0.0) {
        // Special start-up condition. Set b_buffer and interpolate from zero.
        memcpy(device->b_buffer, buffer, NRF_BUFFER_LENGTH);
        device->t = 0.0;
    }

    int j = 0;
    int ii = 0;
    memset(device->iq, 0, 256 * 256 * sizeof(float));
    for (int i = 0; i < length; i += 2) {
        float buffer_pos = i / (float) length;

        int a_i = device->a_buffer[i];
        int a_q = device->a_buffer[i + 1];
        int b_i = device->b_buffer[i];
        int b_q = device->b_buffer[i + 1];
        if (device->device_type == NRF_DEVICE_HACKRF || device->device_type == NRF_DEVICE_DUMMY) {
            a_i = (a_i + 128) % 256;
            a_q = (a_q + 128) % 256;
            b_i = (b_i + 128) % 256;
            b_q = (b_q + 128) % 256;
        }
        float v_i = _nrf_lerp(a_i, b_i, device->t);
        float v_q = _nrf_lerp(a_q, b_q, device->t);

        device->samples[j++] = v_i / 256.0;
        device->samples[j++] = v_q / 256.0;
        device->samples[j++] = buffer_pos;

        int iq_index = (int) (roundf(v_i * 256) + roundf(v_q));
        device->iq[iq_index]++;

        fftw_complex *p = device->fft_in;
        p[ii][0] = v_i / 256.0;
        p[ii][1] = v_q / 256.0;
        ii++;
    }
    fftw_execute(device->fft_plan);
    // Move the previous lines down
    memcpy((char *)&device->fft + FFT_SIZE * sizeof(vec3), &device->fft, FFT_SIZE * (FFT_HISTORY_SIZE - 1) * sizeof(vec3));
    // Set the first line
    for (int i = 0; i < FFT_SIZE; i++) {
        float buffer_pos = i / (float) FFT_SIZE;
        fftw_complex *out = device->fft_out;
        device->fft[i] = vec3_init(out[i][0], out[i][1], buffer_pos);
    }

    device->t += device->t_step;

    return 0;
}

// This function will block, so needs to be called on its own thread.
void *_nrf_rtlsdr_receive_loop(nrf_device *device) {
    while (device->receiving) {
        int n_read;
        int status = rtlsdr_read_sync((rtlsdr_dev_t*) device->device, device->receive_buffer, NRF_BUFFER_LENGTH, &n_read);
        _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_read_sync");
        if (n_read < NRF_BUFFER_LENGTH) {
            fprintf(stderr, "Short read, samples lost, exiting!\n");
            exit(EXIT_FAILURE);
        }
        _nrf_process_sample_block(device, device->receive_buffer, NRF_BUFFER_LENGTH);
    }
    return NULL;
}

static int _nrf_hackrf_receive_sample_block(hackrf_transfer *transfer) {
    nrf_device *device = (nrf_device *)transfer->rx_ctx;
    return _nrf_process_sample_block(device, transfer->buffer, transfer->valid_length);
}

#define MILLISECS 1000000

static void _nrf_sleep_milliseconds(int millis) {
    struct timespec t1, t2;
    t1.tv_sec = 0;
    t1.tv_nsec = millis * MILLISECS;
    nanosleep(&t1 , &t2);
}

static void *_nrf_dummy_receive_loop(nrf_device *device) {
    while (device->receiving) {
        unsigned char *buffer = device->receive_buffer + (device->dummy_block_index * NRF_BUFFER_LENGTH);
        _nrf_process_sample_block(device, buffer, NRF_BUFFER_LENGTH);
        device->dummy_block_index++;
        if (device->dummy_block_index >= device->dummy_block_length) {
            device->dummy_block_index = 0;
        }
        _nrf_sleep_milliseconds(1000 / 60);
    }
    return NULL;
}

static int _nrf_rtlsdr_start(nrf_device *device, double freq_mhz) {
    int status;

    status = rtlsdr_open((rtlsdr_dev_t**)&device->device, 0);
    if (status != 0) {
        return status;
    }

    device->device_type = NRF_DEVICE_RTLSDR;
    device->receive_buffer = calloc(NRF_BUFFER_LENGTH, sizeof(uint8_t));
    device->t_step = 0.1;

    rtlsdr_dev_t* dev = (rtlsdr_dev_t*) device->device;

    status = rtlsdr_set_sample_rate(dev, 2e6);
    _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_set_sample_rate");

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

static int _nrf_hackrf_start(nrf_device *device, double freq_mhz) {
    int status;

    status = hackrf_init();
    _NRF_HACKRF_CHECK_STATUS(NULL, status, "hackrf_init");

    status = hackrf_open((hackrf_device**)&device->device);
    if (status != 0) {
        return status;
    }

    device->device_type = NRF_DEVICE_HACKRF;
    device->t_step = 0.1;

    hackrf_device *dev = (hackrf_device*) device->device;

    freq_mhz = _nrf_clamp_frequency(device, freq_mhz);
    status = hackrf_set_freq(dev, freq_mhz * 1e6);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_freq");

    status = hackrf_set_sample_rate(dev, 5e6);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_sample_rate");

    status = hackrf_set_amp_enable(dev, 0);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_amp_enable");

    status = hackrf_set_lna_gain(dev, 32);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_lna_gain");

    status = hackrf_set_vga_gain(dev, 30);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_lna_gain");

    device->receiving = 1;
    status = hackrf_start_rx(dev, _nrf_hackrf_receive_sample_block, device);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_start_rx");

    return 0;
}

static int _nrf_dummy_start(nrf_device *device, const char *data_file) {
    device->device_type = NRF_DEVICE_DUMMY;
    device->t_step = 0.1;

    fprintf(stderr, "WARN nrf_device_new: Couldn't open SDR device. Falling back on data file %s\n", data_file);
    if (data_file != NULL) {
        FILE *fp = fopen(data_file, "rb");
        if (fp != NULL) {
            fseek(fp, 0L, SEEK_END);
            long size = ftell(fp);
            rewind(fp);
            device->receive_buffer = calloc(size, sizeof(uint8_t));
            device->dummy_block_length = size / NRF_BUFFER_LENGTH;
            device->dummy_block_index = 0;
            fread(device->receive_buffer, size, 1, fp);
            fclose(fp);
        } else {
            fprintf(stderr, "WARN nrf_device_new: Couldn't open %s. Using empty buffer.\n", data_file);
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
    int status;
    nrf_device *device = calloc(1, sizeof(nrf_device));

    device->a_buffer = calloc(NRF_BUFFER_LENGTH, sizeof(uint8_t));
    device->b_buffer = calloc(NRF_BUFFER_LENGTH, sizeof(uint8_t));
    device->t = -1;
    device->t_step = 0.1;

    memset(device->samples, 0, NRF_SAMPLES_SIZE * 3 * sizeof(float));
    device->fft_in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NRF_SAMPLES_SIZE);
    device->fft_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NRF_SAMPLES_SIZE);
    device->fft_plan = fftw_plan_dft_1d(FFT_SIZE, device->fft_in, device->fft_out, FFTW_FORWARD, FFTW_MEASURE);
    memset(device->fft, 0, sizeof(vec3) * FFT_SIZE * FFT_HISTORY_SIZE);

    // Try to find a suitable hardware device, fall back to data file.
    status = _nrf_rtlsdr_start(device, freq_mhz);
    if (status != 0) {
        status = _nrf_hackrf_start(device, freq_mhz);
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
    free(device->a_buffer);
    free(device->b_buffer);
    if (device->receive_buffer) {
        free(device->receive_buffer);
    }
    fftw_destroy_plan(device->fft_plan);
    fftw_free(device->fft_in);
    fftw_free(device->fft_out);
    free(device);
}
