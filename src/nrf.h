// NDBX Radio Frequency functions, based on HackRF

#ifndef NRF_H
#define NRF_H

#include <fftw3.h>
#include <pthread.h>

#include "vec.h"

#define NRF_BUFFER_LENGTH (16 * 16384)
#define NRF_SAMPLES_SIZE 131072
#define FFT_SIZE 1024
#define FFT_HISTORY_SIZE 512

typedef enum {
    NRF_DEVICE_DUMMY = 0,
    NRF_DEVICE_RTLSDR,
    NRF_DEVICE_HACKRF
} nrf_device_type;

typedef struct {
    nrf_device_type device_type;
    void *device;

    pthread_t receive_thread;
    int receiving;

    uint8_t *buffer;
    int buffer_size;

    int dummy_block_length;
    int dummy_block_index;

    float samples[NRF_SAMPLES_SIZE * 3];
    float iq[256 * 256];

    vec3 fft[FFT_SIZE * FFT_HISTORY_SIZE];
    fftw_complex *fft_in;
    fftw_complex *fft_out;
    fftw_plan fft_plan;
} nrf_device;

nrf_device *nrf_device_new(double freq_mhz, const char* data_file);
void nrf_device_set_frequency(nrf_device *device, double freq_mhz);
void nrf_device_free(nrf_device *device);

#endif // NRF_H
