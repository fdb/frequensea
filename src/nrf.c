// NDBX Radio Frequency functions, based on HackRF

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "nrf.h"

hackrf_device *device;

#define HACKRF_CHECK_STATUS(device, status, message) \
if (status != 0) { \
    printf("FAIL: %s\n", message); \
    hackrf_close(device->device); \
    hackrf_exit(); \
    exit(EXIT_FAILURE); \
} \


static int process_sample_block(nrf_device *device, int length, unsigned char *buffer) {
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

static int receive_sample_block(hackrf_transfer *transfer) {
    nrf_device *device = (nrf_device *)transfer->rx_ctx;
    return process_sample_block(device, transfer->valid_length, transfer->buffer);
}

#define MILLISECS 1000000

static void sleep_milliseconds(int millis) {
    struct timespec t1, t2;
    t1.tv_sec = 0;
    t1.tv_nsec = millis * MILLISECS;
    nanosleep(&t1 , &t2);
}

static void *receive_fake_samples(nrf_device *device) {
    while (1) {
        unsigned char *buffer = device->fake_sample_blocks + device->fake_sample_block_index;
        process_sample_block(device, NRF_SAMPLES_SIZE * 2, buffer);
        device->fake_sample_block_index++;
        if (device->fake_sample_block_index >= device->fake_sample_block_size) {
            device->fake_sample_block_index = 0;
        }
        sleep_milliseconds(1000 / 60);
    }
    return NULL;
}

// Start receiving on the given frequency.
// If the device could not be opened, use the raw contents of the data_file
// instead.
nrf_device *nrf_start(double freq_mhz, const char* data_file) {
    int status;

    nrf_device *device = calloc(1, sizeof(nrf_device));

    device->fft_in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NRF_SAMPLES_SIZE);
    device->fft_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NRF_SAMPLES_SIZE);
    device->fft_plan = fftw_plan_dft_1d(FFT_SIZE, device->fft_in, device->fft_out, FFTW_FORWARD, FFTW_MEASURE);
    memset(device->fft, 0, sizeof(vec3) * FFT_SIZE * FFT_HISTORY_SIZE);

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

        pthread_create(&device->fake_sample_receiver_thread, NULL, (void *(*)(void *))&receive_fake_samples, device);
    }

    return device;
}

// Change the frequency to the given frequency, in MHz.
void nrf_freq_set(nrf_device *device, double freq_mhz) {
    if (device->device == NULL) return;
    int status = hackrf_set_freq(device->device, freq_mhz * 1e6);
    HACKRF_CHECK_STATUS(device, status, "hackrf_set_freq");
}

// Stop receiving data
void nrf_stop(nrf_device *device) {
    if (device->device != NULL) {
        hackrf_stop_rx(device->device);
        hackrf_close(device->device);
    }
    if (device->fake_sample_receiver_thread != NULL) {
        pthread_kill(device->fake_sample_receiver_thread, 1);
    }
    hackrf_exit();
    fftw_destroy_plan(device->fft_plan);
    fftw_free(device->fft_in);
    fftw_free(device->fft_out);
    free(device);
}
