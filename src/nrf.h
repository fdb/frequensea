// NDBX Radio Frequency functions, based on HackRF

#ifndef NRF_H
#define NRF_H

#include <libhackrf/hackrf.h>

#define NRF_BUFFER_SIZE 262144

typedef struct {
    hackrf_device *device;
    uint8_t buffer[NRF_BUFFER_SIZE];
} nrf_device;

nrf_device *nrf_start(double freq_mhz);
void nrf_set_freq(nrf_device *device, double freq_mhz);
void nrf_stop(nrf_device *device);

#endif // NRF_H
