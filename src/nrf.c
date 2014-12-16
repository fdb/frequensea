// NDBX Radio Frequency functions, based on HackRF

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libhackrf/hackrf.h>

#define HACKRF_BUFFER_SIZE 262144
uint8_t buffer[HACKRF_BUFFER_SIZE];

hackrf_device *device;

#define HACKRF_CHECK_STATUS(status, message) \
if (status != 0) { \
    printf("FAIL: %s\n", message); \
    hackrf_close(device); \
    hackrf_exit(); \
    exit(-1); \
} \


static int receive_sample_block(hackrf_transfer *transfer) {
    memcpy(buffer, transfer->buffer, transfer->valid_length);
    return 0;
}

// Start receiving on the given frequency
void nrf_start(double freq_mhz) {
    int status;

    status = hackrf_init();
    HACKRF_CHECK_STATUS(status, "hackrf_init");

    status = hackrf_open(&device);
    HACKRF_CHECK_STATUS(status, "hackrf_open");

    status = hackrf_set_freq(device, freq_mhz * 1e6);
    HACKRF_CHECK_STATUS(status, "hackrf_set_freq");

    status = hackrf_set_sample_rate(device, 10e6);
    HACKRF_CHECK_STATUS(status, "hackrf_set_sample_rate");

    status = hackrf_set_amp_enable(device, 0);
    HACKRF_CHECK_STATUS(status, "hackrf_set_amp_enable");

    status = hackrf_set_lna_gain(device, 32);
    HACKRF_CHECK_STATUS(status, "hackrf_set_lna_gain");

    status = hackrf_set_vga_gain(device, 30);
    HACKRF_CHECK_STATUS(status, "hackrf_set_lna_gain");

    status = hackrf_start_rx(device, receive_sample_block, NULL);
    HACKRF_CHECK_STATUS(status, "hackrf_start_rx");

    memset(buffer, 0, HACKRF_BUFFER_SIZE);
}

// Change the frequency to the given frequency, in MHz.
void nrf_set_freq(double freq_mhz) {
    int status = hackrf_set_freq(device, freq_mhz * 1e6);
    HACKRF_CHECK_STATUS(status, "hackrf_set_freq");
}

// Copy the contents of the current HackRF buffer into the given buffer.
// The buffer needs to be at least HACKRF_BUFFER_SIZE
void nrf_read(uint8_t *dst) {
    memcpy(dst, buffer, HACKRF_BUFFER_SIZE);
}

// Stop receiving data
void nrf_stop() {
    hackrf_stop_rx(device);
    hackrf_close(device);
    hackrf_exit();
}
