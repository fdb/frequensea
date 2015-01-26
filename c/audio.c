// Play random noise as audio

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <OpenAL/al.h>
#include <OpenAL/alc.h>

#include <libhackrf/hackrf.h>

const int IN_SAMPLE_RATE = 5e6;
const int OUT_SAMPLE_RATE = 48000;

typedef struct {
    float x;
    float y;
} vec2;

#define HACKRF_CHECK_STATUS(device, status, message) \
if (status != 0) { \
    fprintf(stderr, "HackRF error: %s\n", message); \
    if (device != NULL) hackrf_close(device); \
    hackrf_exit(); \
    exit(EXIT_FAILURE); \
} \

void nal_check_error(const char *file, int line) {
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

#define NAL_CHECK_ERROR() nal_check_error(__FILE__, __LINE__)

ALuint buffer;
ALenum format = AL_FORMAT_MONO8;
const ALsizei freq = OUT_SAMPLE_RATE;
const ALsizei size = freq * 3;
ALuint source;

ALuint *data;
int has_received = 0;

int received_size = 0;
vec2 *vec_buffer;

const int HACKRF_SAMPLES_SIZE = 131072;
const int HACKRF_BUFFER_SIZE = 262144;

const float TAU = M_PI * 2;

const int center_freq = 125000000;
const int desired_freq = 124190000;

// Generates coefficients for a FIR low-pass filter with the given
// half-amplitude frequency and kernel length at the given sample rate.
//
// sample_rate    - The signal's sample rate.
// half_ampl_freq - The half-amplitude frequency in Hz.
// length         - The filter kernel's length. Should be an odd number.
//
// Returns the FIR coefficients for the filter.
float *get_low_pass_fir_coefficients(int sample_rate, int half_ampl_freq, int length) {
    length += (length + 1) % 2;
    float freq = half_ampl_freq / (float) sample_rate;
    float *coefs = calloc(length, sizeof(float));
    int center = floor(length / 2);
    float sum = 0;
    for (int i = 0; i < length; i++) {
        float val;
        if (i == center) {
            val = TAU * freq;
        } else {
            float angle = TAU * (i + 1) / (float) (length + 1);
            val = sin(TAU * freq * (i - center)) / (float) (i - center);
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

typedef struct {
    int length;
    float *coefficients;
    int offset;
    int center;
    int samples_length;
    float *samples;
} fir_filter;

fir_filter *fir_filter_new(int sample_rate, int half_ampl_freq, int length) {
    fir_filter *filter = calloc(1, sizeof(fir_filter));
    filter->length = length;
    filter->coefficients = get_low_pass_fir_coefficients(sample_rate, half_ampl_freq, length);
    filter->offset = length - 1;
    filter->center = floor(length / 2);
    filter->samples_length = filter->offset;
    filter->samples = calloc(filter->samples_length, sizeof(float));
    return filter;
}

void fir_filter_load(fir_filter *filter, float *samples, int length) {
    float *new_samples = calloc(length + filter->offset, sizeof(float));
    memcpy(new_samples, filter->samples, (filter->samples_length - filter->offset) * sizeof(float));
    memcpy(new_samples + filter->offset, samples, length * sizeof(float));
    free(filter->samples);
    filter->samples_length = length + filter->offset;
    filter->samples = new_samples;
}

float fir_filter_get(fir_filter *filter, int index) {
    float v = 0;
    for (int i = 0; i < filter->length; i++) {
        v += filter->coefficients[i] * filter->samples[index + i];
    }
    return v;
}

void fir_filter_free(fir_filter *filter) {
    free(filter->coefficients);
    free(filter->samples);
    free(filter);
}

typedef struct {
    int in_rate;
    int out_rate;
    fir_filter *filter;
    float rate_mul;
    int out_length;
    float *out_samples;
} downsampler;

downsampler *downsampler_new(int in_rate, int out_rate, int filter_freq, int kernel_length) {
    downsampler *d = calloc(1, sizeof(downsampler));
    d->in_rate = in_rate;
    d->out_rate = out_rate;
    d->filter = fir_filter_new(in_rate, filter_freq, kernel_length);
    d->rate_mul = in_rate / (float) out_rate;
    // Fixme: remove HACKRF_SAMPLES_SIZE
    d->out_length = floor(HACKRF_SAMPLES_SIZE / d->rate_mul);
    d->out_samples = calloc(d->out_length, sizeof(float));
    return d;
}

void downsampler_process(downsampler *d, float *samples, int length) {
    fir_filter_load(d->filter, samples, length);
    float t = 0;
    for (int i = 0; i < d->out_length; i++) {
        d->out_samples[i] = fir_filter_get(d->filter, floor(t));
        t += d->rate_mul;
    }
}

void downsampler_free(downsampler *d) {
    fir_filter_free(d->filter);
    free(d->out_samples);
    free(d);
}

float *samples_i;
float *samples_q;

float cosine = 1;
float sine = 0;

downsampler *downsampler_i;
downsampler *downsampler_q;
downsampler *downsampler_audio;

float average(float *values, int length) {
    float sum = 0;
    for (int i = 0; i < length; i++) {
        sum += values[i];
    }
    return sum / length;
}

void shift_frequency(float *samples_i, float *samples_q, int length, int freq_offset, int sample_rate, float *cosine_ptr, float *sine_ptr) {
    float delta_cos = cos(TAU * freq_offset / (float) sample_rate);
    float delta_sin = sin(TAU * freq_offset / (float) sample_rate);
    float cosine = *cosine_ptr;
    float sine = *sine_ptr;
    for (int i = 0; i < length; i++) {
        float vi = samples_i[i];
        float vq = samples_q[i];
        samples_i[i] = vi * cosine - vq * sine;
        samples_q[i] = vi * sine + vq * cosine;
        float new_sine = cosine * delta_sin + sine * delta_cos;
        float new_cosine = cosine * delta_cos - sine * delta_sin;
        sine = new_sine;
        cosine = new_cosine;
    }
    *cosine_ptr = cosine;
    *sine_ptr = sine;
}

int receive_sample_block(hackrf_transfer *transfer) {
    if (received_size > size) return 0;

    // Convert to floats
    for (int i = 0; i < HACKRF_SAMPLES_SIZE; i++) {
        unsigned int vi = transfer->buffer[i * 2];
        unsigned int vq = transfer->buffer[i * 2 + 1];
        samples_i[i] = vi / 128.0 - 0.995;
        samples_q[i] = vq / 128.0 - 0.995;
        //samples_i[i] = (vi - 128.0) / 256.0;
        //samples_q[i] = (vq - 128.0) / 256.0;
    }

    // Shift frequency
    //int freq_offset = desired_freq - center_freq;
    //shift_frequency(samples_i, samples_q, HACKRF_SAMPLES_SIZE, freq_offset, IN_SAMPLE_RATE, &cosine, &sine);

    // Downsample
    downsampler_process(downsampler_i, samples_i, HACKRF_SAMPLES_SIZE);
    downsampler_process(downsampler_q, samples_q, HACKRF_SAMPLES_SIZE);

    // Take average
    float avg_i = average(downsampler_i->out_samples, downsampler_i->out_length);
    float avg_q = average(downsampler_q->out_samples, downsampler_q->out_length);

    // Demodulate
    int out_length = downsampler_i->out_length;
    float *demodulated = calloc(out_length, sizeof(float));

    float sigRatio = IN_SAMPLE_RATE / OUT_SAMPLE_RATE;
    float specSqrSum = 0;
    float sigSqrSum = 0;
    float sigSum = 0;


    for (int i = 0; i < out_length; i++) {
        float iv = downsampler_i->out_samples[i] - avg_i;
        float iq = downsampler_q->out_samples[i] - avg_q;
        float power = iv * iv + iq * iq;
        float ampl = sqrt(power);
        if (i == 0) {
            printf("%.5f\n", ampl);
        }
        demodulated[i] = ampl;

        int origIndex = floor(i * sigRatio);
        float origI = samples_i[origIndex];
        float origQ = samples_q[origIndex];
        specSqrSum += origI * origI + origQ * origQ;
        sigSqrSum += power;
        sigSum += ampl;
    }

    float halfPoint = sigSum / out_length;
    for (int i = 0; i < out_length; i++) {
        demodulated[i] = (demodulated[i] - halfPoint) / halfPoint;
    }
    //float relSignalPower = sigSqrSum / specSqrSum;

    // Downsample again
    downsampler_process(downsampler_audio, demodulated, out_length);
    for (int i = 0; i < out_length; i++) {
        data[received_size++] = (downsampler_audio->out_samples[i] * 5) - 128;
    }

    //memcpy(buffer + , )

    //received_size += out_length;
    printf("Received %.1f%%\n", received_size / (float)size * 100);

    if (received_size > size) {
        alBufferData(buffer, format, data, size, freq);
        NAL_CHECK_ERROR();
        alSourcei(source, AL_BUFFER, buffer);
        NAL_CHECK_ERROR();
        alSourcePlay(source);
        NAL_CHECK_ERROR();

    }
    return 0;
}

int main() {
    int status;
    hackrf_device *hrf;

    data = calloc(size + freq, sizeof(ALuint)); // We need a bit of extra data because we go over the buffer size.
    vec_buffer = calloc(HACKRF_BUFFER_SIZE, sizeof(vec2));

    // Initialize the audio context
    ALCdevice *device = alcOpenDevice(NULL);
    if (!device) {
        fprintf(stderr, "Could not open audio device.\n");
        return 1;
    }
    ALCcontext *ctx = alcCreateContext(device, NULL);
    alcMakeContextCurrent(ctx);

    // Initialize an audio buffer
    alGetError(); // clear error code
    alGenBuffers(1, &buffer);
    NAL_CHECK_ERROR();

    samples_i = calloc(HACKRF_SAMPLES_SIZE, sizeof(float));
    samples_q = calloc(HACKRF_SAMPLES_SIZE, sizeof(float));

    const int INTER_RATE = 48000;
    int bandwidth = 10000;
    int filter_freq = bandwidth / 2;
    downsampler_i = downsampler_new(IN_SAMPLE_RATE, INTER_RATE, filter_freq, 351);
    downsampler_q = downsampler_new(IN_SAMPLE_RATE, INTER_RATE, filter_freq, 351);
    downsampler_audio = downsampler_new(48000, OUT_SAMPLE_RATE, 10000, 41);

    // Fill buffer with random data
    //arc4random_buf(data, size);


    //alBufferData(buffer, format, data, size, freq);

    // Initialize a source
    alGenSources(1, &source);
    NAL_CHECK_ERROR();

    // Attach buffer to source
    //alSourcei(source, AL_BUFFER, buffer);
    //NAL_CHECK_ERROR();

    // Play
    //alSourcePlay(source);
    //NAL_CHECK_ERROR();

    status = hackrf_init();
    HACKRF_CHECK_STATUS(NULL, status, "hackrf_init");

    status = hackrf_open(&hrf);
    HACKRF_CHECK_STATUS(hrf, status, "hackrf_open");

    status = hackrf_set_freq(hrf, 124190000);
    HACKRF_CHECK_STATUS(hrf, status, "hackrf_set_freq");

    status = hackrf_set_sample_rate(hrf, IN_SAMPLE_RATE);
    HACKRF_CHECK_STATUS(hrf, status, "hackrf_set_sample_rate");

    status = hackrf_set_amp_enable(hrf, 0);
    HACKRF_CHECK_STATUS(hrf, status, "hackrf_set_amp_enable");

    status = hackrf_set_lna_gain(hrf, 32);
    HACKRF_CHECK_STATUS(hrf, status, "hackrf_set_lna_gain");

    status = hackrf_set_vga_gain(hrf, 30);
    HACKRF_CHECK_STATUS(hrf, status, "hackrf_set_lna_gain");

    status = hackrf_start_rx(hrf, receive_sample_block, NULL);
    HACKRF_CHECK_STATUS(hrf, status, "hackrf_start_rx");


    // Playing is asynchronous so wait a while
    sleep(3 * 2);

    // Cleanup
    alDeleteBuffers(1, &buffer);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(ctx);
    alcCloseDevice(device);

    hackrf_stop_rx(hrf);
    hackrf_close(hrf);
    hackrf_exit();
    return 0;
}
