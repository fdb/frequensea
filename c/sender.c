#include <libhackrf/hackrf.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int send_sample_block(hackrf_transfer *transfer) {
    arc4random_buf(transfer->buffer, transfer->buffer_length);
    printf("block length: %d\n", transfer->valid_length);
    return 0;
}

int main(int argc, char **argv) {
    int status;
    status = hackrf_init();
    if (status != 0) {
        printf("FAIL: hackrf_init\n");
        hackrf_exit();
        exit(EXIT_FAILURE);
    }

    hackrf_device *device;
    status = hackrf_open(&device);
    if (status != 0) {
        printf("FAIL: hackrf_open\n");
        hackrf_exit();
        exit(EXIT_FAILURE);
    }

    status = hackrf_set_freq(device, 100.9e6);
    if (status != 0) {
        printf("FAIL: hackrf_set_freq: %d\n", status);
        hackrf_close(device);
        hackrf_exit();
        exit(EXIT_FAILURE);
    }

    status = hackrf_set_amp_enable(device, 1);
    if (status != 0) {
        printf("FAIL: hackrf_set_amp_enable: %d\n", status);
        hackrf_close(device);
        hackrf_exit();
        exit(EXIT_FAILURE);
    }

    status = hackrf_set_lna_gain(device, 40);
    if (status != 0) {
        printf("FAIL: hackrf_set_lna_gain: %d\n", status);
        hackrf_close(device);
        hackrf_exit();
        exit(EXIT_FAILURE);
    }

    status = hackrf_set_vga_gain(device, 62);
    if (status != 0) {
        printf("FAIL: hackrf_set_vga_gain: %d\n", status);
        hackrf_close(device);
        hackrf_exit();
        exit(EXIT_FAILURE);
    }

    status = hackrf_set_txvga_gain(device, 47);
    if (status != 0) {
        printf("FAIL: hackrf_set_txvga_gain: %d\n", status);
        hackrf_close(device);
        hackrf_exit();
        exit(EXIT_FAILURE);
    }

    status = hackrf_start_tx(device, send_sample_block, NULL);
    if (status != 0) {
        printf("FAIL: hackrf_start_rx: %d\n", status);
        hackrf_close(device);
        hackrf_exit();
        exit(EXIT_FAILURE);
    }

    sleep(3);

    hackrf_stop_tx(device);
    hackrf_close(device);
    hackrf_exit();
    return 0;
}
