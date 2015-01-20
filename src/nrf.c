// NDBX Radio Frequency functions, based on HackRF

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

static int _nrf_process_sample_block(nrf_device *device, int length, unsigned char *buffer) {
    int j = 0;
    int ii = 0;
    for (int i = 0; i < length; i += 2) {
        float t = i / (float) length;
        int vi = buffer[i];
        int vq = buffer[i + 1];
        vi = (vi + 128) % 256;
        vq = (vq + 128) % 256;
        device->samples[j++] = vi / 256.0f;
        device->samples[j++] = vq / 256.0f;
        device->samples[j++] = t;

        fftw_complex *p = device->fft_in;
        p[ii][0] = buffer[i] / 255.0;
        p[ii][1] = buffer[i + 1] / 255.0;
        ii++;
    }
    fftw_execute(device->fft_plan);
    // Move the previous lines down
    memcpy((char *)&device->fft + FFT_SIZE * sizeof(vec3), &device->fft, FFT_SIZE * (FFT_HISTORY_SIZE - 1) * sizeof(vec3));
    // Set the first line
    for (int i = 0; i < FFT_SIZE; i++) {
        float t = i / (float) FFT_SIZE;
        fftw_complex *out = device->fft_out;
        device->fft[i] = vec3_init(out[i][0], out[i][1], t);
    }
    return 0;
}


static void _nrf_rtlsdr_receive_sample_block(unsigned char *buffer, uint32_t buffer_length, void *ctx) {
    nrf_device *device = (nrf_device *) ctx;
    _nrf_process_sample_block(device, buffer_length, buffer);
}

// This function will block, so needs to be called on its own thread.
void *_nrf_rtlsdr_start_receiving(nrf_device *device) {
    int status = rtlsdr_read_async((rtlsdr_dev_t*)device->device, _nrf_rtlsdr_receive_sample_block, device, 0, 0);
    _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_read_async");
    return NULL;
}

static int _nrf_hackrf_receive_sample_block(hackrf_transfer *transfer) {
    nrf_device *device = (nrf_device *)transfer->rx_ctx;
    return _nrf_process_sample_block(device, transfer->valid_length, transfer->buffer);
}

#define MILLISECS 1000000

static void _nrf_sleep_milliseconds(int millis) {
    struct timespec t1, t2;
    t1.tv_sec = 0;
    t1.tv_nsec = millis * MILLISECS;
    nanosleep(&t1 , &t2);
}

static void *_nrf_receive_fake_samples(nrf_device *device) {
    while (1) {
        unsigned char *buffer = device->fake_sample_blocks + device->fake_sample_block_index;
        _nrf_process_sample_block(device, NRF_SAMPLES_SIZE * 2, buffer);
        device->fake_sample_block_index++;
        if (device->fake_sample_block_index >= device->fake_sample_block_size) {
            device->fake_sample_block_index = 0;
        }
        _nrf_sleep_milliseconds(1000 / 60);
    }
    return NULL;
}

static int _nrf_rtlsdr_start(nrf_device *device, double freq_mhz) {
    int status;

    const char *device_name = rtlsdr_get_device_name(0);
    printf("Device %s\n", device_name);

    status = rtlsdr_open((rtlsdr_dev_t**)&device->device, 0);
    if (status != 0) {
        return status;
    }

    device->device_type = NRF_DEVICE_RTLSDR;

    rtlsdr_dev_t* dev = (rtlsdr_dev_t*) device->device;

    status = rtlsdr_set_sample_rate(dev, 2e6);
    _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_set_sample_rate");

    // Set auto-gain mode
    status = rtlsdr_set_tuner_gain_mode(dev, 0);
    _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_set_tuner_gain_mode");

    status = rtlsdr_set_agc_mode(dev, 1);
    _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_set_agc_mode");

    status = rtlsdr_set_center_freq(dev, freq_mhz * 1e6);
    _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_set_center_freq");

    status = rtlsdr_reset_buffer(dev);
    _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_reset_buffer");

    printf("Start\n");
    pthread_create(&device->receive_thread, NULL, (void *(*)(void *)) &_nrf_rtlsdr_start_receiving, device);
    printf("Running\n");

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

    hackrf_device *dev = (hackrf_device*) device->device;

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

    status = hackrf_start_rx(dev, _nrf_hackrf_receive_sample_block, device);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_start_rx");

    return 0;
}

static int _nrf_dummy_start(nrf_device *device, const char *data_file) {
    device->device_type = NRF_DEVICE_DUMMY;
    fprintf(stderr, "WARN nrf_start: Couldn't open SDR device. Falling back on data file %s\n", data_file);
    uint8_t int_buffer[NRF_SAMPLES_SIZE * 2];
    memset(int_buffer, 0, NRF_SAMPLES_SIZE * 2);
    if (data_file != NULL) {
        FILE *fp = fopen(data_file, "rb");
        if (fp != NULL) {
            fseek(fp, 0L, SEEK_END);
            long size = ftell(fp);
            rewind(fp);
            device->fake_sample_block_size = size / (131072 * 2);
            device->fake_sample_block_index = 0;
            device->fake_sample_blocks = calloc(size, sizeof(unsigned char));
            fread(device->fake_sample_blocks, size, 1, fp);
            fclose(fp);
        } else {
            fprintf(stderr, "WARN nrf_start: Couldn't open %s. Using empty buffer.\n", data_file);
        }
    }

    pthread_create(&device->receive_thread, NULL, (void *(*)(void *))&_nrf_receive_fake_samples, device);

    return 0;
}


// Start receiving on the given frequency.
// If the device could not be opened, use the raw contents of the data_file
// instead.
nrf_device *nrf_start(double freq_mhz, const char* data_file) {
    int status;
    nrf_device *device = calloc(1, sizeof(nrf_device));

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
                fprintf(stderr, "ERROR nrf_start: Couldn't even start dummy device. Exiting.\n");
            }
        }
    }

    return device;
}

// Change the frequency to the given frequency, in MHz.
void nrf_freq_set(nrf_device *device, double freq_mhz) {
    if (device->device_type == NRF_DEVICE_RTLSDR) {
        int status = rtlsdr_set_center_freq((rtlsdr_dev_t*) device->device, freq_mhz * 1e6);
        _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_set_center_freq");
    } else if (device->device_type == NRF_DEVICE_HACKRF) {
        int status = hackrf_set_freq(device->device, freq_mhz * 1e6);
        _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_freq");
    }
}

// Stop receiving data
void nrf_stop(nrf_device *device) {
    if (device->device_type == NRF_DEVICE_RTLSDR) {
        rtlsdr_cancel_async((rtlsdr_dev_t*) device->device);
        pthread_kill(device->receive_thread, 1);
        rtlsdr_close((rtlsdr_dev_t*) device);
    } else if (device->device_type == NRF_DEVICE_HACKRF) {
        hackrf_stop_rx((hackrf_device*) device->device);
        hackrf_close((hackrf_device*) device->device);
        hackrf_exit();
    }
    fftw_destroy_plan(device->fft_plan);
    fftw_free(device->fft_in);
    fftw_free(device->fft_out);
    free(device);
}
