// NDBX Radio Frequency functions, based on HackRF

#ifndef NRF_H
#define NRF_H

#include <libhackrf/hackrf.h>

#define NRF_SAMPLES_SIZE 131072

typedef struct {
    hackrf_device *device;
    float samples[NRF_SAMPLES_SIZE * 3];
} nrf_device;

nrf_device *nrf_start(double freq_mhz, const char* data_file);
void nrf_set_freq(nrf_device *device, double freq_mhz);
void nrf_stop(nrf_device *device);

#endif // NRF_H
