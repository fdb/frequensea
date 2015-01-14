// NDBX Radio Frequency functions, based on HackRF

#ifndef NRF_H
#define NRF_H

#include <libhackrf/hackrf.h>

#include "vec.h"

#define NRF_SAMPLES_SIZE 131072

typedef struct {
    vec3 *samples;
    int length;
    int start;
    int end;
} nrf_buffer;

typedef struct {
    hackrf_device *device;
    vec3 samples[NRF_SAMPLES_SIZE];
    nrf_buffer buffer;
} nrf_device;

nrf_device *nrf_start(double freq_mhz, const char* data_file);
void nrf_freq_set(nrf_device *device, double freq_mhz);
void nrf_stop(nrf_device *device);
void nrf_samples_consume(nrf_device *device, int sample_count);

#endif // NRF_H
