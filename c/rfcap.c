// Capture n samples of a frequency range.

#include <libhackrf/hackrf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const int SAMPLE_SIZE=512*512;

double current_freq = 0;
int n_samples = 0;

hackrf_device *device;
int receive_count = 0;
uint8_t buffer[SAMPLE_SIZE];

#define HACKRF_CHECK_STATUS(status, message) \
    if (status != 0) { \
        printf("FAIL: %s\n", message); \
        hackrf_close(device); \
        hackrf_exit(); \
        exit(EXIT_FAILURE); \
    } \

void export_buffer(uint8_t *buffer) {
    char fname[100];
    snprintf(fname, 100, "export/rf-%.3f-%d.raw", current_freq, receive_count + 1);
    printf("%s\n", fname);
    FILE *write_ptr = fopen(fname, "wb");
    fwrite(buffer, SAMPLE_SIZE, 1, write_ptr);
    fclose(write_ptr);
}


int receive_sample_block(hackrf_transfer *transfer) {
    memcpy(buffer, transfer->buffer, SAMPLE_SIZE);
    export_buffer(buffer);
    receive_count += 1;
    return 0;
}

int main(int argc, char **argv) {
    int status;

    if (argc != 3) {
        printf("Usage: rfcap FREQ N_SAMPLES\n");
        exit(EXIT_FAILURE);
    }

    current_freq = atof(argv[1]);
    n_samples = atoi(argv[2]);

    status = hackrf_init();
    HACKRF_CHECK_STATUS(status, "hackrf_init");

    status = hackrf_open(&device);
    HACKRF_CHECK_STATUS(status, "hackrf_open");

    status = hackrf_set_freq(device, current_freq * 1e6);
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

    while (receive_count < n_samples) {
        usleep(100);
    }

    hackrf_stop_rx(device);
    hackrf_close(device);
    hackrf_exit();
    return 0;
}
