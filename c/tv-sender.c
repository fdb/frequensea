#include <libhackrf/hackrf.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../externals/stb/stb_image.h"

uint8_t *image_data;

const int freq = 49e6;

int send_sample_block(hackrf_transfer *transfer) {
    //printf(".\n");
    //arc4random_buf(transfer->buffer, transfer->buffer_length);
    memset(transfer->buffer, 0, transfer->buffer_length);

    //memcpy(transfer->buffer, image_data, transfer->buffer_length);

    int v = 0;
    for (int i = 0; i < transfer->buffer_length; i += 2) {
        int px = image_data[v];
        int vi = 255-px ;
        int vq = 255-px ;
        //printf("%d %d %d\n", px, vi, vq);
        transfer->buffer[i] = vi;
        transfer->buffer[i + 1] = vq;
        v+= 3;
    }
    //printf("%d\n", v);
    //     //double vi = i;
    //     //vi /= (double) transfer->buffer_length;
    //     //vi *= 255;
    //     //vi *= M_PI * 2;
    //     //vi = sin(vi);
    //     //vi = abs(vi) * 255;
    //     //printf("%d\n", transfer->buffer[i]);
    //     //transfer->buffer[i] = 0;
    //     transfer->buffer[i+1] = 255;
    //     transfer->buffer[i+2] = 128;
    //     transfer->buffer[i+3] = 0;
    // }
    //printf("block length: %d\n", transfer->valid_length);
    return 0;
}


int main(int argc, char **argv) {
    int width, height, channels;
    image_data  = stbi_load("../img/lenna-tv.jpg", &width, &height, &channels, 3);
    printf("width %d height %d channels %d\n", width, height, channels);

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

    status = hackrf_set_freq(device, freq);
    if (status != 0) {
        printf("FAIL: hackrf_set_freq: %d\n", status);
        hackrf_close(device);
        hackrf_exit();
        exit(EXIT_FAILURE);
    }

    status = hackrf_set_sample_rate(device, 6e6);
    if (status != 0) {
        printf("FAIL: hackrf_set_sample_rate: %d\n", status);
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

    // // status = hackrf_set_lna_gain(device, 40);
    // // if (status != 0) {
    // //     printf("FAIL: hackrf_set_lna_gain: %d\n", status);
    // //     hackrf_close(device);
    // //     hackrf_exit();
    // //     exit(EXIT_FAILURE);
    // // }

    // // status = hackrf_set_vga_gain(device, 62);
    // // if (status != 0) {
    // //     printf("FAIL: hackrf_set_vga_gain: %d\n", status);
    // //     hackrf_close(device);
    // //     hackrf_exit();
    // //     exit(EXIT_FAILURE);
    // // }

    // status = hackrf_set_txvga_gain(device, 47);
    // if (status != 0) {
    //     printf("FAIL: hackrf_set_txvga_gain: %d\n", status);
    //     hackrf_close(device);
    //     hackrf_exit();
    //     exit(EXIT_FAILURE);
    // }

    status = hackrf_start_tx(device, send_sample_block, NULL);
    if (status != 0) {
        printf("FAIL: hackrf_start_rx: %d\n", status);
        hackrf_close(device);
        hackrf_exit();
        exit(EXIT_FAILURE);
    }

    for (int i = 170.6e6; i < 230e6; i += 0.001e6) {
        printf("Freq %d\n", i);
        status = hackrf_set_freq(device, i);

        sleep(1);
    }
    //sleep(9999);

    hackrf_stop_tx(device);
    hackrf_close(device);
    hackrf_exit();
    return 0;
}
