// Process frequency range in batch.

#include <libhackrf/hackrf.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "easypng.h"

#define WIDTH 512
#define HEIGHT 512

const double start_freq = 1;
const double end_freq = 6000;
const double freq_step = 1;
double current_freq = start_freq;

hackrf_device *device;
int receive_count = 0;
int paused = 0;
uint8_t buffer[WIDTH * HEIGHT];

#define HACKRF_CHECK_STATUS(status, message) \
    if (status != 0) { \
        printf("FAIL: %s\n", message); \
        hackrf_close(device); \
        hackrf_exit(); \
        exit(-1); \
    } \

void export_buffer() {
    paused = 1;
    char fname[100];
    snprintf(fname, 100, "export/img-%.3f.png", current_freq);
    write_gray_png(fname, WIDTH, HEIGHT, buffer);
    paused = 0;
}

void next_frequency() {
    receive_count = 0;
    current_freq += freq_step;
    if (current_freq <= end_freq) {
        printf("Frequency: %.3f MHz\n", current_freq);
        int status = hackrf_set_freq(device, current_freq * 1e6);
        HACKRF_CHECK_STATUS(status, "hackrf_set_freq");
    } else {
        printf("Done.\n");
        exit(0);
    }
}

int receive_sample_block(hackrf_transfer *transfer) {
    if (paused) return 0;
    receive_count += 1;
    // memcpy(buffer, transfer->buffer, transfer->valid_length);
    for (int i = 0; i < transfer->valid_length; i+= 2) {
        buffer[i] = transfer->buffer[i + 1];
        buffer[i + 1] = transfer->buffer[i + 1];
    }
    return 0;
}

int main(int argc, char **argv) {
    int status;

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

    while (1) {
        if (receive_count < 10) {
            usleep(100);
        } else {
            export_buffer();
            next_frequency();
        }
    }

    hackrf_stop_rx(device);
    hackrf_close(device);
    hackrf_exit();
    return 0;
}
