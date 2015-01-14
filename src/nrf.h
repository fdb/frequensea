// NDBX Radio Frequency functions, based on HackRF

#ifndef NRF_H
#define NRF_H

#include <libhackrf/hackrf.h>
#include <fftw3.h>
#include <pthread.h>

#include "vec.h"

#define NRF_SAMPLES_SIZE 131072
#define FFT_SIZE 1024
#define FFT_HISTORY_SIZE 512

typedef struct {
    hackrf_device *device;
    float samples[NRF_SAMPLES_SIZE * 3];
    vec3 fft[FFT_SIZE * FFT_HISTORY_SIZE];
    fftw_complex *fft_in;
    fftw_complex *fft_out;
    fftw_plan fft_plan;
    pthread_t fake_sample_receiver_thread;
    unsigned char *fake_sample_blocks;
    int fake_sample_block_size;
    int fake_sample_block_index;
} nrf_device;

nrf_device *nrf_start(double freq_mhz, const char* data_file);
void nrf_freq_set(nrf_device *device, double freq_mhz);
void nrf_stop(nrf_device *device);

#endif // NRF_H
