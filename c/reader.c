#include <libhackrf/hackrf.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const long NRF_SAMPLES_LENGTH = 262144;
const long BUFFER_SIZE = NRF_SAMPLES_LENGTH * 1;
const int CENTER_FREQ = 433e6;
const int SAMPLE_RATE = 10e6;
uint8_t buffer[BUFFER_SIZE];
long buffer_pos = 0;

int skip = 10;

#define CHECK_STATUS(status, message) \
    if (status != 0) { \
        printf("FAIL: %s\n", message); \
        hackrf_close(device); \
        hackrf_exit(); \
        exit(EXIT_FAILURE); \
    } \

int receive_sample_block(hackrf_transfer *transfer) {
    if (skip > 0) {
        printf("skip: %d\n", skip);
        skip--;
        return 0;
    }
    printf("block length: %d index: %d\n", transfer->valid_length, (int)(buffer_pos / (double) NRF_SAMPLES_LENGTH));
    for (int i = 0; i < transfer->valid_length; i++) {
        if (buffer_pos < BUFFER_SIZE) {
            buffer[buffer_pos++] = transfer->buffer[i];
        }
    }
    if (buffer_pos >= BUFFER_SIZE) {
        char fname[100];
        snprintf(fname, 100, "../rfdata/rf-%.3f.raw", CENTER_FREQ / 1.0e6);
        FILE *fp = fopen(fname, "wb");
        if (fp) {
            fwrite(buffer, BUFFER_SIZE, 1, fp);
            fclose(fp);
            printf("Written %s.\n", fname);
            exit(0);
        }
    }
    return 0;
}

int main(int argc, char **argv) {

    int status;
    hackrf_device *device;

    status = hackrf_init();
    CHECK_STATUS(status, "hackrf_init");

    status = hackrf_open(&device);
    CHECK_STATUS(status, "hackrf_open");

    status = hackrf_set_freq(device, CENTER_FREQ);
    CHECK_STATUS(status, "hackrf_set_freq");

    status = hackrf_set_sample_rate(device, SAMPLE_RATE);
    CHECK_STATUS(status, "hackrf_set_sample_rate");

    status = hackrf_set_amp_enable(device, 0);
    CHECK_STATUS(status, "hackrf_set_amp_enable");

    status = hackrf_set_lna_gain(device, 32);
    CHECK_STATUS(status, "hackrf_set_lna_gain");

    status = hackrf_set_vga_gain(device, 34);
    CHECK_STATUS(status, "hackrf_set_vga_gain");

    status = hackrf_start_rx(device, receive_sample_block, NULL);
    CHECK_STATUS(status, "hackrf_start_rx");

    while (buffer_pos < BUFFER_SIZE) {
        sleep(1);
    }

    hackrf_stop_rx(device);
    hackrf_close(device);
    hackrf_exit();
    return 0;
}
