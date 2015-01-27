// NDBX Radio Frequency functions, based on HackRF

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <libhackrf/hackrf.h>
#include <rtl-sdr.h>

#include "nrf.h"

const double TAU = M_PI * 2;

void _nrf_rtlsdr_check_status(nrf_device *device, int status, const char *message, const char *file, int line) {
    if (status != 0) {
        fprintf(stderr, "RTL-SDR: %s (Status code %d) %s:%d\n", message, status, file, line);
        if (device == NULL && device->device != NULL) {

            rtlsdr_close((rtlsdr_dev_t*) device->device);
        }
        exit(EXIT_FAILURE);
    }
}

#define _NRF_RTLSDR_CHECK_STATUS(device, status, message) _nrf_rtlsdr_check_status(device, status, message, __FILE__, __LINE__)

void _nrf_hackrf_check_status(nrf_device *device, int status, const char *message, const char *file, int line) {
    if (status != 0) {
        fprintf(stderr, "NRF HackRF fatal error: %s\n", message);
        if (device != NULL && device->device != NULL) {
            hackrf_close((hackrf_device*) device->device);
        }
        hackrf_exit();
        exit(EXIT_FAILURE);
    }
}

#define _NRF_HACKRF_CHECK_STATUS(device, status, message) _nrf_hackrf_check_status(device, status, message, __FILE__, __LINE__)

static int _nrf_lerp(float a, float b, float t) {
    return a * (1.0 - t) + b * t;
}

// Limit the frequency range to a possible value.
double _nrf_clamp_frequency(nrf_device *device, double freq_mhz) {
    if (device->device_type == NRF_DEVICE_RTLSDR) {
        return freq_mhz < 10 ? 10 : freq_mhz > 1766 ? 1766 : freq_mhz;
    } else if (device->device_type == NRF_DEVICE_HACKRF) {
        return freq_mhz < 1 ? 1 : freq_mhz > 6000 ? 6000 : freq_mhz;
    } else {
        return freq_mhz;
    }
}

static int _nrf_process_sample_block(nrf_device *device, unsigned char *buffer, int length) {
    assert(length == NRF_BUFFER_LENGTH);

    if (device->t >= 1.0) {
        memcpy(device->a_buffer, device->b_buffer, NRF_BUFFER_LENGTH);
        memcpy(device->b_buffer, buffer, NRF_BUFFER_LENGTH);
        device->t = 0.0;
    } else if (device->t < 0.0) {
        // Special start-up condition. Set b_buffer and interpolate from zero.
        memcpy(device->b_buffer, buffer, NRF_BUFFER_LENGTH);
        device->t = 0.0;
    }

    int j = 0;
    int ii = 0;
    memset(device->iq, 0, 256 * 256 * sizeof(float));
    for (int i = 0; i < length; i += 2) {
        float buffer_pos = i / (float) length;

        int a_i = device->a_buffer[i];
        int a_q = device->a_buffer[i + 1];
        int b_i = device->b_buffer[i];
        int b_q = device->b_buffer[i + 1];
        if (device->device_type == NRF_DEVICE_HACKRF || device->device_type == NRF_DEVICE_DUMMY) {
            a_i = (a_i + 128) % 256;
            a_q = (a_q + 128) % 256;
            b_i = (b_i + 128) % 256;
            b_q = (b_q + 128) % 256;
        }
        float v_i = _nrf_lerp(a_i, b_i, device->t);
        float v_q = _nrf_lerp(a_q, b_q, device->t);

        device->samples[j++] = v_i / 256.0;
        device->samples[j++] = v_q / 256.0;
        device->samples[j++] = buffer_pos;

        int iq_index = (int) (roundf(v_i * 256) + roundf(v_q));
        device->iq[iq_index]++;

        fftw_complex *p = device->fft_in;
        p[ii][0] = v_i / 256.0;
        p[ii][1] = v_q / 256.0;
        ii++;
    }
    fftw_execute(device->fft_plan);
    // Move the previous lines down
    memcpy((char *)&device->fft + FFT_SIZE * sizeof(vec3), &device->fft, FFT_SIZE * (FFT_HISTORY_SIZE - 1) * sizeof(vec3));
    // Set the first line
    for (int i = 0; i < FFT_SIZE; i++) {
        float buffer_pos = i / (float) FFT_SIZE;
        fftw_complex *out = device->fft_out;
        device->fft[i] = vec3_init(out[i][0], out[i][1], buffer_pos);
    }

    device->t += device->t_step;

    return 0;
}

// This function will block, so needs to be called on its own thread.
void *_nrf_rtlsdr_receive_loop(nrf_device *device) {
    while (device->receiving) {
        int n_read;
        int status = rtlsdr_read_sync((rtlsdr_dev_t*) device->device, device->receive_buffer, NRF_BUFFER_LENGTH, &n_read);
        _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_read_sync");
        if (n_read < NRF_BUFFER_LENGTH) {
            fprintf(stderr, "Short read, samples lost, exiting!\n");
            exit(EXIT_FAILURE);
        }
        _nrf_process_sample_block(device, device->receive_buffer, NRF_BUFFER_LENGTH);
    }
    return NULL;
}

static int _nrf_hackrf_receive_sample_block(hackrf_transfer *transfer) {
    nrf_device *device = (nrf_device *)transfer->rx_ctx;
    return _nrf_process_sample_block(device, transfer->buffer, transfer->valid_length);
}

#define MILLISECS 1000000

static void _nrf_sleep_milliseconds(int millis) {
    struct timespec t1, t2;
    t1.tv_sec = 0;
    t1.tv_nsec = millis * MILLISECS;
    nanosleep(&t1 , &t2);
}

static void *_nrf_dummy_receive_loop(nrf_device *device) {
    while (device->receiving) {
        unsigned char *buffer = device->receive_buffer + (device->dummy_block_index * NRF_BUFFER_LENGTH);
        _nrf_process_sample_block(device, buffer, NRF_BUFFER_LENGTH);
        device->dummy_block_index++;
        if (device->dummy_block_index >= device->dummy_block_length) {
            device->dummy_block_index = 0;
        }
        _nrf_sleep_milliseconds(1000 / 60);
    }
    return NULL;
}

static const int RTLSDR_DEFAULT_SAMPLE_RATE = 2e6;

static int _nrf_rtlsdr_start(nrf_device *device, double freq_mhz) {
    int status;

    status = rtlsdr_open((rtlsdr_dev_t**)&device->device, 0);
    if (status != 0) {
        return status;
    }

    device->device_type = NRF_DEVICE_RTLSDR;
    device->receive_buffer = calloc(NRF_BUFFER_LENGTH, sizeof(uint8_t));

    rtlsdr_dev_t* dev = (rtlsdr_dev_t*) device->device;

    status = rtlsdr_set_sample_rate(dev, 2e6);
    _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_set_sample_rate");
    device->sample_rate = RTLSDR_DEFAULT_SAMPLE_RATE;

    // Set auto-gain mode
    status = rtlsdr_set_tuner_gain_mode(dev, 0);
    _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_set_tuner_gain_mode");

    status = rtlsdr_set_agc_mode(dev, 1);
    _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_set_agc_mode");

    freq_mhz = _nrf_clamp_frequency(device, freq_mhz);
    status = rtlsdr_set_center_freq(dev, freq_mhz * 1e6);
    _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_set_center_freq");

    status = rtlsdr_reset_buffer(dev);
    _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_reset_buffer");

    device->receiving = 1;
    pthread_create(&device->receive_thread, NULL, (void *(*)(void *)) &_nrf_rtlsdr_receive_loop, device);

    return 0;
}

static const int HACKRF_DEFAULT_SAMPLE_RATE = 5e6;

static int _nrf_hackrf_start(nrf_device *device, double freq_mhz) {
    int status;

    status = hackrf_init();
    _NRF_HACKRF_CHECK_STATUS(NULL, status, "hackrf_init");

    status = hackrf_open((hackrf_device**)&device->device);
    if (status != 0) {
        return status;
    }

    device->device_type = NRF_DEVICE_HACKRF;

    hackrf_device *dev = (hackrf_device*) device->device;

    freq_mhz = _nrf_clamp_frequency(device, freq_mhz);
    status = hackrf_set_freq(dev, freq_mhz * 1e6);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_freq");

    status = hackrf_set_sample_rate(dev, HACKRF_DEFAULT_SAMPLE_RATE);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_sample_rate");
    device->sample_rate = HACKRF_DEFAULT_SAMPLE_RATE;

    status = hackrf_set_amp_enable(dev, 0);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_amp_enable");

    status = hackrf_set_lna_gain(dev, 32);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_lna_gain");

    status = hackrf_set_vga_gain(dev, 30);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_lna_gain");

    device->receiving = 1;
    status = hackrf_start_rx(dev, _nrf_hackrf_receive_sample_block, device);
    _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_start_rx");

    return 0;
}

static const int DUMMY_DEFAULT_SAMPLE_RATE = 5e6;

static int _nrf_dummy_start(nrf_device *device, const char *data_file) {
    device->device_type = NRF_DEVICE_DUMMY;
    device->sample_rate = DUMMY_DEFAULT_SAMPLE_RATE;

    fprintf(stderr, "WARN nrf_device_new: Couldn't open SDR device. Falling back on data file %s\n", data_file);
    if (data_file != NULL) {
        FILE *fp = fopen(data_file, "rb");
        if (fp != NULL) {
            fseek(fp, 0L, SEEK_END);
            long size = ftell(fp);
            rewind(fp);
            device->receive_buffer = calloc(size, sizeof(uint8_t));
            device->dummy_block_length = size / NRF_BUFFER_LENGTH;
            device->dummy_block_index = 0;
            fread(device->receive_buffer, size, 1, fp);
            fclose(fp);
        } else {
            fprintf(stderr, "WARN nrf_device_new: Couldn't open %s. Using empty buffer.\n", data_file);
            device->receive_buffer = calloc(NRF_BUFFER_LENGTH, sizeof(uint8_t));
            device->dummy_block_length = 1;
            device->dummy_block_index = 0;
        }
    }

    device->receiving = 1;
    pthread_create(&device->receive_thread, NULL, (void *(*)(void *))&_nrf_dummy_receive_loop, device);

    return 0;
}

// Start receiving on the given frequency.
// If the device could not be opened, use the raw contents of the data_file
// instead.
nrf_device *nrf_device_new(double freq_mhz, const char* data_file, float interpolate_step) {
    int status;
    nrf_device *device = calloc(1, sizeof(nrf_device));

    device->a_buffer = calloc(NRF_BUFFER_LENGTH, sizeof(uint8_t));
    device->b_buffer = calloc(NRF_BUFFER_LENGTH, sizeof(uint8_t));
    device->t = -1;
    device->t_step = interpolate_step;

    memset(device->samples, 0, NRF_SAMPLES_SIZE * 3 * sizeof(float));
    device->fft_in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NRF_SAMPLES_SIZE);
    device->fft_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NRF_SAMPLES_SIZE);
    device->fft_plan = fftw_plan_dft_1d(FFT_SIZE, device->fft_in, device->fft_out, FFTW_FORWARD, FFTW_MEASURE);
    memset(device->fft, 0, sizeof(vec3) * FFT_SIZE * FFT_HISTORY_SIZE);

    // Try to find a suitable hardware device, fall back to data file.
    status = _nrf_rtlsdr_start(device, freq_mhz);
    if (status != 0) {
        status = _nrf_hackrf_start(device, freq_mhz);
        if (status != 0) {
            status = _nrf_dummy_start(device, data_file);
            if (status != 0) {
                fprintf(stderr, "ERROR nrf_device_new: Couldn't even start dummy device. Exiting.\n");
            }
        }
    }

    return device;
}

// Change the frequency to the given frequency, in MHz.
double nrf_device_set_frequency(nrf_device *device, double freq_mhz) {
    freq_mhz = _nrf_clamp_frequency(device, freq_mhz);
    if (device->device_type == NRF_DEVICE_RTLSDR) {
        int status = rtlsdr_set_center_freq((rtlsdr_dev_t*) device->device, freq_mhz * 1e6);
        _NRF_RTLSDR_CHECK_STATUS(device, status, "rtlsdr_set_center_freq");
    } else if (device->device_type == NRF_DEVICE_HACKRF) {
        int status = hackrf_set_freq(device->device, freq_mhz * 1e6);
        _NRF_HACKRF_CHECK_STATUS(device, status, "hackrf_set_freq");
    }
    return freq_mhz;
}

// Stop receiving data
void nrf_device_free(nrf_device *device) {
    if (device->device_type == NRF_DEVICE_RTLSDR) {
        device->receiving = 0;
        pthread_join(device->receive_thread, NULL);
        rtlsdr_close((rtlsdr_dev_t*) device->device);
    } else if (device->device_type == NRF_DEVICE_HACKRF) {
        hackrf_stop_rx((hackrf_device*) device->device);
        hackrf_close((hackrf_device*) device->device);
        hackrf_exit();
    } else if (device->device_type == NRF_DEVICE_DUMMY) {
        device->receiving = 0;
        pthread_join(device->receive_thread, NULL);
    }
    free(device->a_buffer);
    free(device->b_buffer);
    if (device->receive_buffer) {
        free(device->receive_buffer);
    }
    fftw_destroy_plan(device->fft_plan);
    fftw_free(device->fft_in);
    fftw_free(device->fft_out);
    free(device);
}

// Finite Impulse Response (FIR) Filter

// Generates coefficients for a FIR low-pass filter with the given
// half-amplitude frequency and kernel length at the given sample rate.
//
// sample_rate    - The signal's sample rate.
// half_ampl_freq - The half-amplitude frequency in Hz.
// length         - The filter kernel's length. Should be an odd number.
//
// Returns the FIR coefficients for the filter.
double *nrf_fir_get_low_pass_coefficients(int sample_rate, int half_ampl_freq, int length) {
    length += (length + 1) % 2;
    double freq = half_ampl_freq / (double) sample_rate;
    double *coefs = calloc(length, sizeof(double));
    int center = floor(length / 2);
    double sum = 0;
    for (int i = 0; i < length; i++) {
        double val;
        if (i == center) {
            val = TAU * freq;
        } else {
            double angle = TAU * (i + 1) / (double) (length + 1);
            val = sin(TAU * freq * (i - center)) / (double) (i - center);
            val *= 0.42 - 0.5 * cos(angle) + 0.08 * cos(2 * angle);
        }
        sum += val;
        coefs[i] = val;
    }
    for (int i = 0; i < length; i++) {
        coefs[i] /= sum;
    }
    return coefs;
}

nrf_fir_filter *nrf_fir_filter_new(int sample_rate, int half_ampl_freq, int length) {
    nrf_fir_filter *filter = calloc(1, sizeof(nrf_fir_filter));
    filter->length = length;
    filter->coefficients = nrf_fir_get_low_pass_coefficients(sample_rate, half_ampl_freq, length);
    filter->offset = length - 1;
    filter->center = floor(length / 2);
    filter->samples_length = filter->offset;
    filter->samples = calloc(filter->samples_length, sizeof(double));
    return filter;
}

void nrf_fir_filter_load(nrf_fir_filter *filter, double *samples, int length) {
    int should_free = 0;
    int new_length = length + filter->offset;
    double *new_samples;
    if (filter->samples_length != new_length) {
        should_free = 1;
        new_samples = calloc(new_length, sizeof(double));
    } else {
        new_samples = filter->samples;
    }

    // Copy the last `offset` samples to the new buffer.
    memcpy(new_samples, filter->samples + filter->samples_length - filter->offset, filter->offset * sizeof(double));
    memcpy(new_samples + filter->offset, samples, length * sizeof(double));

    if (should_free) {
        free(filter->samples);
    }
    filter->samples = new_samples;
    filter->samples_length = new_length;
}

double nrf_fir_filter_get(nrf_fir_filter *filter, int index) {
    double v = 0;
    for (int i = 0; i < filter->length; i++) {
        v += filter->coefficients[i] * filter->samples[index + i];
    }
    return v;
}

void nrf_fir_filter_free(nrf_fir_filter *filter) {
    free(filter->coefficients);
    free(filter->samples);
    free(filter);
}

// Downsampler

nrf_downsampler *nrf_downsampler_new(int in_rate, int out_rate, int filter_freq, int kernel_length) {
    nrf_downsampler *d = calloc(1, sizeof(nrf_downsampler));
    d->in_rate = in_rate;
    d->out_rate = out_rate;
    d->filter = nrf_fir_filter_new(in_rate, filter_freq, kernel_length);
    d->rate_mul = in_rate / (double) out_rate;
    d->out_length = 0;
    d->out_samples = NULL;
    return d;
}

void nrf_downsampler_process(nrf_downsampler *d, double *samples, int length) {
    nrf_fir_filter_load(d->filter, samples, length);

    // FIXME: Optimize by comparing out_length to d->out_length
    free(d->out_samples);
    d->out_length = floor(length / d->rate_mul);
    d->out_samples = calloc(d->out_length, sizeof(double));

    double t = 0;
    for (int i = 0; i < d->out_length; i++) {
        d->out_samples[i] = nrf_fir_filter_get(d->filter, floor(t));
        t += d->rate_mul;
    }
}

void nrf_downsampler_free(nrf_downsampler *d) {
    nrf_fir_filter_free(d->filter);
    free(d->out_samples);
    free(d);
}

// Frequency shifter

nrf_freq_shifter *nrf_freq_shifter_new(int freq_offset, int sample_rate) {
    nrf_freq_shifter *shifter = calloc(1, sizeof(nrf_freq_shifter));
    shifter->freq_offset = freq_offset;
    shifter->sample_rate = sample_rate;
    shifter->cosine = 1;
    shifter->sine = 1;
    return shifter;
}

void nrf_freq_shifter_process(nrf_freq_shifter *shifter, double *samples_i, double *samples_q, int length) {
    double delta_cos = cos(TAU * shifter->freq_offset / (double) shifter->sample_rate);
    double delta_sin = sin(TAU * shifter->freq_offset / (double) shifter->sample_rate);
    int cosine = shifter->cosine;
    int sine = shifter->sine;
    for (int i = 0; i < length; i++) {
        double vi = samples_i[i];
        double vq = samples_q[i];
        samples_i[i] = vi * cosine - vq * sine;
        samples_q[i] = vi * sine + vq * cosine;
        double new_sine = cosine * delta_sin + sine * delta_cos;
        double new_cosine = cosine * delta_cos - sine * delta_sin;
        sine = new_sine;
        cosine = new_cosine;
    }
    shifter->cosine = cosine;
    shifter->sine = sine;
}

void nrf_freq_shifter_free(nrf_freq_shifter *shifter) {
    free(shifter);
}

// FM Demodulator

nrf_fm_demodulator *nrf_fm_demodulator_new(int in_sample_rate, int out_sample_rate) {
    nrf_fm_demodulator *d = calloc(1, sizeof(nrf_fm_demodulator));
    const int inter_rate = 336000;
    const int  max_f = 75000;
    const double filter = max_f * 0.8;
    d->in_sample_rate = in_sample_rate;
    d->out_sample_rate = out_sample_rate;
    d->ampl_conv = out_sample_rate / (TAU * max_f);
    int filter_freq = filter * 0.8;
    d->downsampler_i = nrf_downsampler_new(in_sample_rate, inter_rate, filter_freq, 51);
    d->downsampler_q = nrf_downsampler_new(in_sample_rate, inter_rate, filter_freq, 51);
    d->downsampler_audio = nrf_downsampler_new(inter_rate, out_sample_rate, 10000, 41);
    return d;
}

void nrf_fm_demodulator_process(nrf_fm_demodulator *demodulator, double *samples_i, double *samples_q, int length) {
    // Downsample
    nrf_downsampler_process(demodulator->downsampler_i, samples_i, length);
    nrf_downsampler_process(demodulator->downsampler_q, samples_q, length);

    // Allocate demodulated samples buffer
    int demodulated_length = demodulator->downsampler_i->out_length;
    if (demodulated_length != demodulator->demodulated_length) {
        free(demodulator->demodulated_samples);
        demodulator->demodulated_samples = calloc(demodulated_length, sizeof(double));
        demodulator->demodulated_length = demodulated_length;
    }
    double *demodulated_samples = demodulator->demodulated_samples;

    // Actual FM demodulation
    double prev = 0;
    double delta_sum_squared = 0;
    double l_i = demodulator->l_i;
    double l_q = demodulator->l_q;
    for (int i = 0; i < demodulated_length; i++) {
        double real = l_i * demodulator->downsampler_i->out_samples[i] + l_q * demodulator->downsampler_q->out_samples[i];
        double imag = l_i * demodulator->downsampler_q->out_samples[i] - demodulator->downsampler_i->out_samples[i] * l_q;
        double sgn = 1;
        if (imag < 0) {
            sgn *= -1;
            imag *= -1;
        }
        double ang = 0;
        double div;
        if (real == imag) {
            div = 1;
        } else if (real > imag) {
            div = imag / real;
        } else {
            ang = -M_PI / 2;
            div = real / imag;
            sgn *= -1;
        }
        demodulated_samples[i] = sgn *
            (ang + div
                / (0.98419158358617365
                    + div * (0.093485702629671305
                        + div * 0.19556307900617517))) * demodulator->ampl_conv;
        l_i = demodulator->downsampler_i->out_samples[i];
        l_q = demodulator->downsampler_q->out_samples[i];
        double delta = prev - demodulated_samples[i];
        delta_sum_squared += delta * delta;
        prev = demodulated_samples[i];
    }
    demodulator->l_i = l_i;
    demodulator->l_q = l_q;

    // Downsample again, for audio
    nrf_downsampler_process(demodulator->downsampler_audio, demodulated_samples, demodulated_length);

   // Copy audio samples to demodulator
    int audio_length = demodulator->downsampler_audio->out_length;
    if (audio_length != demodulator->audio_length) {
        free(demodulator->audio_samples);
        demodulator->audio_samples = calloc(audio_length, sizeof(double));
        demodulator->audio_length = audio_length;
    }
    memcpy(demodulator->audio_samples, demodulator->downsampler_audio->out_samples, audio_length * sizeof(double));
    double *audio_samples = demodulator->audio_samples;

    // De-emphasize samples
    double alpha = 1.0 / (1.0 + demodulator->out_sample_rate * 50.0 / 1e6);
    double val = demodulator->deemphasis_val;
    for (int i = 0; i < audio_length; i++) {
        val = val + alpha * (audio_samples[i] - val);
        audio_samples[i] = val;
    }
    demodulator->deemphasis_val = val;
}

void nrf_fm_demodulator_free(nrf_fm_demodulator *demodulator) {
    nrf_downsampler_free(demodulator->downsampler_i);
    nrf_downsampler_free(demodulator->downsampler_q);
    nrf_downsampler_free(demodulator->downsampler_audio);
    free(demodulator->demodulated_samples);
    free(demodulator->audio_samples);
    free(demodulator);
}

// Decoder

nrf_decoder *nrf_decoder_new(nrf_demodulate_type demodulate_type, int in_sample_rate, int out_sample_rate, int freq_offset) {
    nrf_decoder *decoder = calloc(1, sizeof(nrf_decoder));
    decoder->demodulate_type = demodulate_type;
    if (decoder->demodulate_type == NRF_DEMODULATE_WBFM) {
        decoder->demodulator = nrf_fm_demodulator_new(decoder->in_sample_rate, decoder->out_sample_rate);
    }
    decoder->in_sample_rate = in_sample_rate;
    decoder->out_sample_rate = out_sample_rate;
    decoder->freq_shifter = nrf_freq_shifter_new(freq_offset, in_sample_rate);
    return decoder;
}

void nrf_decoder_process(nrf_decoder *decoder, uint8_t *buffer, size_t length) {
    // Convert 8-bit samples to doubles
    if (decoder->samples_length != length) {
        free(decoder->samples_i);
        free(decoder->samples_q);
        decoder->samples_i = calloc(length, sizeof(double));
        decoder->samples_q = calloc(length, sizeof(double));
    }

    double *samples_i = decoder->samples_i;
    double *samples_q = decoder->samples_q;
    for (int i = 0; i < length; i++) {
        unsigned int vi = buffer[i * 2];
        unsigned int vq = buffer[i * 2 + 1];
        samples_i[i] = vi / 128.0 - 0.995;
        samples_q[i] = vq / 128.0 - 0.995;
        //samples_i[i] = (vi - 128.0) / 128.0;
        //samples_q[i] = (vq - 128.0) / 128.0;
    }

    // Shift frequency
    nrf_freq_shifter_process(decoder->freq_shifter, samples_i, samples_q, length);

    // Demodulate
    if (decoder->demodulate_type == NRF_DEMODULATE_WBFM) {
        nrf_fm_demodulator *demodulator = (nrf_fm_demodulator*) decoder->demodulator;
        nrf_fm_demodulator_process(demodulator, samples_i, samples_q, length);
    }
}

void nrf_decoder_free(nrf_decoder *decoder) {
    nrf_freq_shifter_free(decoder->freq_shifter);
    free(decoder);
}

// Audio Player

static const int AUDIO_SAMPLE_RATE = 48000;
static const int FREQ_OFFSET = 50000;

static void _nrf_al_check_error(const char *file, int line) {
    ALenum err = alGetError();
    int has_error = 0;
    while (err != AL_NO_ERROR) {
        has_error = 1;
        char *msg = NULL;
        switch (err) {
            case AL_INVALID_NAME:
                msg = "AL_INVALID_NAME";
                break;
            case AL_INVALID_ENUM:
                msg = "AL_INVALID_ENUM";
                break;
            case AL_INVALID_VALUE:
                msg = "AL_INVALID_VALUE";
                break;
            case AL_INVALID_OPERATION:
                msg = "AL_INVALID_OPERATION";
                break;
            case AL_OUT_OF_MEMORY:
                msg = "AL_OUT_OF_MEMORY";
                break;
        }
        fprintf(stderr, "OpenAL error: %s - %s:%d\n", msg, file, line);
        err = alGetError();
    }
    if (has_error) {
        exit(EXIT_FAILURE);
    }
}

#define _NRF_AL_CHECK_ERROR() _nrf_al_check_error(__FILE__, __LINE__)

nrf_player *nrf_player_new(nrf_device *device, nrf_demodulate_type demodulate_type) {
    nrf_player *player = calloc(1, sizeof(nrf_player));
    player->device = device;
    player->decoder = nrf_decoder_new(demodulate_type, device->sample_rate, AUDIO_SAMPLE_RATE, FREQ_OFFSET);

     // Initialize the audio context
    player->audio_device = alcOpenDevice(NULL);
    if (!device) {
        fprintf(stderr, "Could not open audio device.\n");
        exit(EXIT_FAILURE);
    }
    player->audio_context = alcCreateContext(player->audio_device, NULL);
    alcMakeContextCurrent(player->audio_context);
    _NRF_AL_CHECK_ERROR();

    // Initialize an audio source.
    alGenSources(1, &player->audio_source);
    _NRF_AL_CHECK_ERROR();

    // Turn off looping.
    alSourcei(player->audio_source, AL_LOOPING, AL_FALSE);
    _NRF_AL_CHECK_ERROR();

    return player;
}

void nrf_player_free(nrf_player *player) {
    player->shutting_down = 1;
    alcMakeContextCurrent(NULL);
    alcDestroyContext(player->audio_context);
    alcCloseDevice(player->audio_device);
    player->shutting_down = 1;

    // Note we don't own the NRF device, so we're not going to free it.
    free(player->decoder);
    free(player);
}
