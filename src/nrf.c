// NDBX Radio Frequency functions, based on HackRF

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nrf.h"

hackrf_device *device;

#define HACKRF_CHECK_STATUS(device, status, message) \
if (status != 0) { \
    printf("FAIL: %s\n", message); \
    hackrf_close(device->device); \
    hackrf_exit(); \
    exit(-1); \
} \


static int receive_sample_block(hackrf_transfer *transfer) {
    nrf_device *device = (nrf_device *)transfer->rx_ctx;
    int j = 0;
    for (int i = 0; i < transfer->valid_length; i += 2) {
        float t = i / (float) transfer->valid_length;
        int vi = transfer->buffer[i];
        int vq = transfer->buffer[i + 1];
        vi = (vi + 128) % 256;
        vq = (vq + 128) % 256;
        device->samples[j++] = vi / 256.0f;
        device->samples[j++] = vq / 256.0f;
        device->samples[j++] = t;
    }
    return 0;
}

// Start receiving on the given frequency
nrf_device *nrf_start(double freq_mhz) {
    int status;

    nrf_device *device = malloc(sizeof(nrf_device));

    status = hackrf_init();
    HACKRF_CHECK_STATUS(device, status, "hackrf_init");

    status = hackrf_open(&device->device);
    HACKRF_CHECK_STATUS(device, status, "hackrf_open");

    status = hackrf_set_freq(device->device, freq_mhz * 1e6);
    HACKRF_CHECK_STATUS(device, status, "hackrf_set_freq");

    status = hackrf_set_sample_rate(device->device, 10e6);
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

    return device;
}

// Change the frequency to the given frequency, in MHz.
void nrf_set_freq(nrf_device *device, double freq_mhz) {
    int status = hackrf_set_freq(device->device, freq_mhz * 1e6);
    HACKRF_CHECK_STATUS(device, status, "hackrf_set_freq");
}

// Stop receiving data
void nrf_stop(nrf_device *device) {
    hackrf_stop_rx(device->device);
    hackrf_close(device->device);
    hackrf_exit();
    free(device);
}
