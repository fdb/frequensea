// Batch export of FFT data as images.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include <libhackrf/hackrf.h>
#include <fftw3.h>

#include "easypng.h"

const uint32_t FFT_SIZE = 256;
const uint32_t FFT_HISTORY_SIZE = 4096;
const uint32_t SAMPLES_SIZE = 131072;
const uint64_t FREQUENCY_START = 660.00001e6;
const uint64_t FREQUENCY_END = 3010.00001e6;
const uint32_t FREQUENCY_STEP = 5e6;
const uint32_t SAMPLE_RATE = 5e6;
const uint32_t SAMPLE_BLOCKS_TO_SKIP = 10;

uint64_t frequency = FREQUENCY_START;

fftw_complex *fft_in;
fftw_complex *fft_out;
fftw_complex *fft_history;
int history_rows = 0;
fftw_plan fft_plan;
hackrf_device *device;
int skip = SAMPLE_BLOCKS_TO_SKIP;
time_t start_time, end_time;

// Utility ////////////////////////////////////////////////////////////////////

uint8_t clamp_u8(int v, uint8_t min, uint8_t max) {
    return (uint8_t) (v < min ? min : v > max ? max : v);
}

// HackRF /////////////////////////////////////////////////////////////////////

static void hackrf_check_status(int status, const char *message, const char *file, int line) {
    if (status != 0) {
        fprintf(stderr, "NRF HackRF fatal error: %s\n", message);
        if (device != NULL) {
            hackrf_close(device);
        }
        hackrf_exit();
        exit(EXIT_FAILURE);
    }
}

#define HACKRF_CHECK_STATUS(status, message) hackrf_check_status(status, message, __FILE__, __LINE__)

int receive_sample_block(hackrf_transfer *transfer) {
    uint64_t local_frequency = frequency;
    if (skip > 0) {
        skip--;
        return 0;
    }
    if (history_rows >= FFT_HISTORY_SIZE) return 0;
    int ii = 0;
    for (int i = 0; i < SAMPLES_SIZE; i += 2) {
        int vi = (transfer->buffer[i] + 128) % 256;
        int vq = (transfer->buffer[i + 1] + 128) % 256;
        fft_in[ii][0] = powf(-1, ii) * vi / 256.0;
        fft_in[ii][1] = powf(-1, ii) * vq / 256.0;
        ii++;
    }
    fftw_execute(fft_plan);

    // Move one line down.
    memcpy(fft_history + FFT_SIZE, fft_history, FFT_SIZE * (FFT_HISTORY_SIZE - 1) * sizeof(fftw_complex));
    // Set the first line.
    memcpy(fft_history, fft_out, FFT_SIZE * sizeof(fftw_complex));

    history_rows++;
    printf("\r%.f%%", history_rows / (float)FFT_HISTORY_SIZE * 100);
    fflush(stdout);
    if (history_rows >= FFT_HISTORY_SIZE) {
        printf("\n");
        // Write image.
        uint8_t *buffer = calloc(FFT_SIZE * FFT_HISTORY_SIZE, sizeof(uint8_t));
        for (int y = 0; y < FFT_HISTORY_SIZE; y++) {
            for (int x = 0; x < FFT_SIZE; x++) {
                double ci = fft_history[y * FFT_SIZE + x][0];
                double cq = fft_history[y * FFT_SIZE + x][1];
                double pwr = ci * ci + cq * cq;
                //double pwr_dbfs = 10.0 * log2(pwr + 1.0e-20) / log2(2.7182818284);
                double pwr_dbfs = 10.0 * log10(pwr + 1.0e-20);
                pwr_dbfs = pwr_dbfs * 5;
                uint8_t v = clamp_u8(pwr_dbfs, 0, 255);
                if (x == FFT_SIZE / 2) {
                    v = buffer[y * FFT_SIZE + x - 1];
                }
                buffer[y * FFT_SIZE + x] = v;
            }
        }
        char file_name[100];
        snprintf(file_name, 100, "broad-%.0f.png", local_frequency / 1.0e6);
        write_gray_png(file_name, FFT_SIZE, FFT_HISTORY_SIZE, buffer);

        free(buffer);
    }

    return 0;
}

static void setup_hackrf() {
    int status;

    status = hackrf_init();
    HACKRF_CHECK_STATUS(status, "hackrf_init");

    status = hackrf_open(&device);
    HACKRF_CHECK_STATUS(status, "hackrf_open");

    status = hackrf_set_freq(device, frequency);
    HACKRF_CHECK_STATUS(status, "hackrf_set_freq");

    status = hackrf_set_sample_rate(device, SAMPLE_RATE);
    HACKRF_CHECK_STATUS(status, "hackrf_set_sample_rate");

    status = hackrf_set_amp_enable(device, 0);
    HACKRF_CHECK_STATUS(status, "hackrf_set_amp_enable");

    status = hackrf_set_lna_gain(device, 32);
    HACKRF_CHECK_STATUS(status, "hackrf_set_lna_gain");

    status = hackrf_set_vga_gain(device, 40);
    HACKRF_CHECK_STATUS(status, "hackrf_set_lna_gain");

    status = hackrf_start_rx(device, receive_sample_block, NULL);
    HACKRF_CHECK_STATUS(status, "hackrf_start_rx");
}

static void teardown_hackrf() {
    hackrf_stop_rx(device);
    hackrf_close(device);
    hackrf_exit();
}

// FFTW /////////////////////////////////////////////////////////////////////

static void setup_fftw() {
    fft_in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * SAMPLES_SIZE);
    fft_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * SAMPLES_SIZE);
    fft_history = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * FFT_SIZE * FFT_HISTORY_SIZE);
    fft_plan = fftw_plan_dft_1d(FFT_SIZE, fft_in, fft_out, FFTW_FORWARD, FFTW_ESTIMATE);
}

static void teardown_fftw() {
    fftw_destroy_plan(fft_plan);
    fftw_free(fft_in);
    fftw_free(fft_out);
    fftw_free(fft_history);
}

// Main /////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
    setup_fftw();
    setup_hackrf();
    printf("Frequency: %.4f MHz\n", frequency / 1.0e6);
    time(&start_time);

    while (frequency <= FREQUENCY_END) {
        while (history_rows < FFT_HISTORY_SIZE) {
            sleep(1);
        }

        time(&end_time);
        printf("Elapsed: %.0f seconds.\n", difftime(end_time, start_time));

        frequency = frequency + FREQUENCY_STEP;
        if (frequency > FREQUENCY_END) {
            exit(0);
        }

        int status = hackrf_set_freq(device, frequency);
        HACKRF_CHECK_STATUS(status, "hackrf_set_freq");

        skip = SAMPLE_BLOCKS_TO_SKIP;
        history_rows = 0;
        printf("Frequency: %.4f MHz\n", frequency / 1.0e6);
        time(&start_time);
    }

    teardown_hackrf();
    teardown_fftw();

    return 0;
}
