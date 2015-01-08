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

// Start receiving on the given frequency.
// If the device could not be opened, use the raw contents of the data_file
// instead.
nrf_device *nrf_start(double freq_mhz, const char* data_file) {
    int status;

    nrf_device *device = malloc(sizeof(nrf_device));

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
                fread(int_buffer, NRF_SAMPLES_SIZE * 2, 1, fp);
                fclose(fp);
            } else {
                fprintf(stderr, "WARN nrf_start: Couldn't open %s. Using empty buffer.\n", data_file);
            }
        }
        int j = 0;
        for (int i = 0; i < NRF_SAMPLES_SIZE * 2; i += 2) {
            float t = i / (float) NRF_SAMPLES_SIZE * 2;
            int vi = int_buffer[i];
            int vq = int_buffer[i + 1];
            vi = (vi + 128) % 256;
            vq = (vq + 128) % 256;
            device->samples[j++] = vi / 256.0f;
            device->samples[j++] = vq / 256.0f;
            device->samples[j++] = t;
        }
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
    hackrf_exit();
    free(device);
}
