// NDBX Software Defined Radio library

#ifndef NRF_H
#define NRF_H

#include <fftw3.h>
#include <pthread.h>
#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif // __APPLE__

#include "vec.h"

#define NRF_BUFFER_LENGTH (16 * 16384)
#define NRF_SAMPLES_SIZE 131072
#define NRF_IQ_RESOLUTION 256
#define DEFAULT_FFT_SIZE 128
#define DEFAULT_FFT_HISTORY_SIZE 128

// Block

#define NRF_BLOCK_MAX_OUTPUTS 10

typedef enum {
    NRF_BLOCK_SOURCE = 1,
    NRF_BLOCK_GENERIC,
    NRF_BLOCK_SINK
} nrf_block_type;

typedef struct nrf_block nrf_block;

typedef void (*nrf_block_process_fn)(nrf_block *block, void *input_data);

struct nrf_block {
    nrf_block_type type;
    nrf_block_process_fn process_fn;
    int n_outputs;
    void* outputs[NRF_BLOCK_MAX_OUTPUTS];
};

void nrf_block_init(nrf_block* block, nrf_block_type type, nrf_block_process_fn process_fn);
void nrf_block_connect(nrf_block* input, nrf_block* output);
void nrf_block_process(nrf_block* block, void* input_data);

// Buffer

typedef struct {
    int width;
    int height;
    int channels;
    int size_bytes;
    uint8_t *data;
} nrf_buffer;

nrf_buffer *nrf_buffer_new(int width, int height, int channels, const uint8_t *data);
void nrf_buffer_free(nrf_buffer *buffer);

// Device

typedef struct {
    int sample_rate;
    double freq_mhz;
    const char* data_file;
} nrf_device_config;

typedef enum {
    NRF_DEVICE_DUMMY = 0,
    NRF_DEVICE_RTLSDR,
    NRF_DEVICE_HACKRF
} nrf_device_type;

typedef struct nrf_device nrf_device;

typedef void (*nrf_device_decode_cb_fn)(nrf_device *device, void *ctx);

struct nrf_device {
    nrf_device_type device_type;
    void *device;
    int sample_rate;

    nrf_device_decode_cb_fn decode_cb_fn;
    void *decode_cb_ctx;

    pthread_t receive_thread;
    pthread_mutex_t data_mutex;
    int receiving;
    int paused;

    uint8_t *receive_buffer;
    int dummy_block_length;
    int dummy_block_index;

    uint8_t samples[NRF_BUFFER_LENGTH];
};

nrf_device *nrf_device_new(double freq_mhz, const char* data_file);
nrf_device *nrf_device_new_with_config(nrf_device_config config);
double nrf_device_set_frequency(nrf_device *device, double freq_mhz);
void nrf_device_set_decode_handler(nrf_device *device, nrf_device_decode_cb_fn fn, void *ctx);
void nrf_device_set_paused(nrf_device *device, int paused);
void nrf_device_step(nrf_device *device);
nrf_buffer *nrf_device_get_samples_buffer(nrf_device *device);
nrf_buffer *nrf_device_get_iq_buffer(nrf_device *device);
nrf_buffer *nrf_device_get_iq_lines(nrf_device *device, int size_multiplier, float line_percentage);
nrf_buffer *nrf_device_get_fft_buffer(nrf_device *device);
void nrf_device_free(nrf_device *device);

// FFT Analysis

typedef struct {
    int fft_size;
    int fft_history_size;
    double *buffer;
    fftw_complex *fft_in;
    fftw_complex *fft_out;
    fftw_plan fft_plan;
} nrf_fft;

nrf_fft *nrf_fft_new(int fft_size, int fft_history_size);
void nrf_fft_process(nrf_fft *fft, nrf_buffer *buffer);
void nrf_fft_free(nrf_fft *fft);

// Finite Impulse Response (FIR) Filter

typedef struct {
    int length;
    double *coefficients;
    int offset;
    int center;
    int samples_length;
    double *samples;
} nrf_fir_filter;

double *nrf_fir_get_low_pass_coefficients(int sample_rate, int half_ampl_freq, int length);
nrf_fir_filter *nrf_fir_filter_new(int sample_rate, int half_ampl_freq, int length);
void nrf_fir_filter_load(nrf_fir_filter *filter, double *samples, int length);
double nrf_fir_filter_get(nrf_fir_filter *filter, int index);
void nrf_fir_filter_free(nrf_fir_filter *filter);

// Downsampler

typedef struct {
    int in_rate;
    int out_rate;
    nrf_fir_filter *filter;
    double rate_mul;
    int out_length;
    double *out_samples;
} nrf_downsampler;

nrf_downsampler *nrf_downsampler_new(int in_rate, int out_rate, int filter_freq, int kernel_length);
void nrf_downsampler_process(nrf_downsampler *d, double *samples, int length);
void nrf_downsampler_free(nrf_downsampler *d);

// Frequency shifter

typedef struct {
    int freq_offset;
    int sample_rate;
    double cosine;
    double sine;
} nrf_freq_shifter;

nrf_freq_shifter *nrf_freq_shifter_new(int freq_offset, int sample_rate);
void nrf_freq_shifter_process(nrf_freq_shifter *shifter, double *samples_i, double *samples_q, int length);
void nrf_freq_shifter_free(nrf_freq_shifter *shifter);

// RAW Demodulator

typedef struct {
    int in_sample_rate;
    int out_sample_rate;
    nrf_downsampler *downsampler_audio;
    double *audio_samples;
    int audio_samples_length;
} nrf_raw_demodulator;

nrf_raw_demodulator *nrf_raw_demodulator_new(int in_sample_rate, int out_sample_rate);
void nrf_raw_demodulator_process(nrf_raw_demodulator *demodulator, double *samples_i, double *samples_q, int length);
void nrf_raw_demodulator_free(nrf_raw_demodulator *demodulator);

// FM Demodulator

typedef struct {
    int in_sample_rate;
    int out_sample_rate;
    double ampl_conv;
    double l_i;
    double l_q;
    double deemphasis_val;
    nrf_downsampler *downsampler_i;
    nrf_downsampler *downsampler_q;
    nrf_downsampler *downsampler_audio;
    double *demodulated_samples;
    int demodulated_length;
    double *audio_samples;
    int audio_samples_length;
} nrf_fm_demodulator;

nrf_fm_demodulator *nrf_fm_demodulator_new(int in_sample_rate, int out_sample_rate);
void nrf_fm_demodulator_process(nrf_fm_demodulator *demodulator, double *samples_i, double *samples_q, int length);
void nrf_fm_demodulator_free(nrf_fm_demodulator *demodulator);

// Decoder

typedef enum {
    NRF_DEMODULATE_RAW = 0,
    NRF_DEMODULATE_WBFM
} nrf_demodulate_type;

typedef struct {
    int in_sample_rate;
    int out_sample_rate;
    nrf_demodulate_type demodulate_type;
    void *demodulator;
    nrf_freq_shifter *freq_shifter;
    double *samples_i;
    double *samples_q;
    int samples_length;
    double *audio_samples;
    int audio_samples_length;
} nrf_decoder;

nrf_decoder *nrf_decoder_new(nrf_demodulate_type demodulate_type, int in_sample_rate, int out_sample_rate, int freq_offset);
void nrf_decoder_process(nrf_decoder *decoder, uint8_t *buffer, size_t length);

// Player

typedef struct {
    int size;
    int capacity;
    ALuint *values;
} _nrf_buffer_queue;

typedef struct {
    nrf_demodulate_type demodulate_type;
    nrf_device *device;
    nrf_decoder *decoder;
    ALCcontext *audio_context;
    ALCdevice *audio_device;
    ALuint audio_source;
    _nrf_buffer_queue *audio_buffer_queue;
    int is_playing;
    int shutting_down;
} nrf_player;

nrf_player *nrf_player_new(nrf_device *device, nrf_demodulate_type demodulate_type, int freq_offset);
void nrf_player_set_freq_offset(nrf_player *player, int freq_offset);
void nrf_player_free(nrf_player *player);

#endif // NRF_H
