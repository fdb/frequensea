// Play random noise as audio

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <OpenAL/al.h>
#include <OpenAL/alc.h>

#include <libhackrf/hackrf.h>
#include <sndfile.h>

const double TAU = M_PI * 2;
const int IN_SAMPLE_RATE = 10e6;
const int OUT_SAMPLE_RATE = 48000;
const int DESIRED_FREQ = 100900000;
const int FREQ_OFFSET = 50000;
const int CENTER_FREQ = DESIRED_FREQ + FREQ_OFFSET;
const int SECONDS = 3;
const int DESIRED_OUT_SAMPLES = SECONDS * OUT_SAMPLE_RATE;
const ALenum AL_BUFFER_FORMAT = AL_FORMAT_MONO16;

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

ALuint audio_buffer = -1;

ALuint source;

ALshort *audio_buffer_samples;
int has_received = 0;

int received_samples = 0;


// Generates coefficients for a FIR low-pass filter with the given
// half-amplitude frequency and kernel length at the given sample rate.
//
// sample_rate    - The signal's sample rate.
// half_ampl_freq - The half-amplitude frequency in Hz.
// length         - The filter kernel's length. Should be an odd number.
//
// Returns the FIR coefficients for the filter.
double *get_low_pass_fir_coefficients(int sample_rate, int half_ampl_freq, int length) {
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

typedef struct {
    int length;
    double *coefficients;
    int offset;
    int center;
    int samples_length;
    double *samples;
} fir_filter;

fir_filter *fir_filter_new(int sample_rate, int half_ampl_freq, int length) {
    fir_filter *filter = calloc(1, sizeof(fir_filter));
    filter->length = length;
    filter->coefficients = get_low_pass_fir_coefficients(sample_rate, half_ampl_freq, length);
    filter->offset = length - 1;
    filter->center = floor(length / 2);
    filter->samples_length = filter->offset;
    filter->samples = calloc(filter->samples_length, sizeof(double));
    return filter;
}

void fir_filter_load(fir_filter *filter, double *samples, int length) {
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

double fir_filter_get(fir_filter *filter, int index) {
    double v = 0;
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
    double rate_mul;
    int out_length;
    double *out_samples;
} downsampler;

downsampler *downsampler_new(int in_rate, int out_rate, int filter_freq, int kernel_length) {
    downsampler *d = calloc(1, sizeof(downsampler));
    d->in_rate = in_rate;
    d->out_rate = out_rate;
    d->filter = fir_filter_new(in_rate, filter_freq, kernel_length);
    d->rate_mul = in_rate / (double) out_rate;
    printf("downsampler_new in %d out %d mul %f\n", in_rate, out_rate, d->rate_mul);
    d->out_length = 0;
    d->out_samples = NULL;
    return d;
}

void downsampler_process(downsampler *d, double *samples, int length) {
    fir_filter_load(d->filter, samples, length);

    free(d->out_samples);
    d->out_length = floor(length / d->rate_mul);
    d->out_samples = calloc(d->out_length, sizeof(double));

    double t = 0;
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


double cosine = 1;
double sine = 0;

downsampler *downsampler_i;
downsampler *downsampler_q;
downsampler *downsampler_audio;

double l_i = 0;
double l_q = 0;

double average(double *values, int length) {
    double sum = 0;
    for (int i = 0; i < length; i++) {
        sum += values[i];
    }
    return sum / length;
}

void shift_frequency(double *samples_i, double *samples_q, int length, int freq_offset, int sample_rate, double *cosine_ptr, double *sine_ptr) {
    double delta_cos = cos(TAU * freq_offset / (double) sample_rate);
    double delta_sin = sin(TAU * freq_offset / (double) sample_rate);
    double cosine = *cosine_ptr;
    double sine = *sine_ptr;
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
    *cosine_ptr = cosine;
    *sine_ptr = sine;
}

void process_sample_block(uint8_t *buffer, size_t length) {
    if (received_samples > DESIRED_OUT_SAMPLES) {
        return;
    }

    double *samples_i = calloc(length, sizeof(double));
    double *samples_q = calloc(length, sizeof(double));

    // Convert to doubles
    for (int i = 0; i < length; i++) {
        unsigned int vi = buffer[i * 2];
        unsigned int vq = buffer[i * 2 + 1];
        samples_i[i] = vi / 128.0 - 0.995;
        samples_q[i] = vq / 128.0 - 0.995;
        //samples_i[i] = (vi - 128.0) / 256.0;
        //samples_q[i] = (vq - 128.0) / 256.0;
    }


    printf("A %f %f %f %f\n", samples_i[0], samples_q[0], samples_i[1], samples_q[1]);

    // Shift frequency
    shift_frequency(samples_i, samples_q, length, FREQ_OFFSET, IN_SAMPLE_RATE, &cosine, &sine);

    printf("B %f %f %f %f\n", samples_i[0], samples_q[0], samples_i[1], samples_q[1]);

    // Downsample
    downsampler_process(downsampler_i, samples_i, length);
    downsampler_process(downsampler_q, samples_q, length);

    //printf("C %.9f %.9f %.9f %.9f\n", downsampler_i->out_samples[0], downsampler_q->out_samples[0], downsampler_i->out_samples[1], downsampler_q->out_samples[1]);

    for (int i=0;i<10;i++) {
        printf("C %.9f %.9f\n", downsampler_i->out_samples[i], downsampler_q->out_samples[i]);
    }


    // Demodulate
    int out_length = downsampler_i->out_length;
    double *demodulated = calloc(out_length, sizeof(double));

    double prev = 0;
    double difSqrSum = 0;

    double AMPL_CONV = 336000 / (2 * M_PI * 75000);

    for (int i = 0; i < out_length; i++) {
        double real = l_i * downsampler_i->out_samples[i] + l_q * downsampler_q->out_samples[i];
        double imag = l_i * downsampler_q->out_samples[i] - downsampler_i->out_samples[i] * l_q;
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
        demodulated[i] = sgn *
            (ang + div
                / (0.98419158358617365
                    + div * (0.093485702629671305
                        + div * 0.19556307900617517))) * AMPL_CONV;
        l_i = downsampler_i->out_samples[i];
        l_q = downsampler_q->out_samples[i];
        double dif = prev - demodulated[i];
        difSqrSum += dif * dif;
        prev = demodulated[i];
    }

    for (int i=0;i<10;i++) {
        printf("D %.9f\n", demodulated[i]);
    }

    printf("demod length %d\n", out_length);

    // Downsample again
    downsampler_process(downsampler_audio, demodulated, out_length);
    for (int i=0;i<10;i++) {
        printf("E %.9f\n", downsampler_audio->out_samples[i]);
    }

    printf("downsample audio length %d\n", downsampler_audio->out_length);
    for (int i = 0; i < downsampler_audio->out_length; i++) {
        if (received_samples > DESIRED_OUT_SAMPLES) {
            break;
        }
        double f_sample = downsampler_audio->out_samples[i];
        int16_t i_sample = f_sample * 32000;
        audio_buffer_samples[received_samples++] = i_sample;
    }

    //received_samples += out_length;
    printf("Received %.1f%%\n", received_samples / (double) DESIRED_OUT_SAMPLES * 100);

    if (received_samples > DESIRED_OUT_SAMPLES) {
        printf("Received: %d\n", received_samples);
        // Write out the final sound as 16-bit signed PCM samples at 48000 Hz
        // FILE *fp = fopen("out-out.raw", "w");
        // fwrite(audio_buffer_samples, sizeof(int16_t), received_samples * sizeof(ALshort), fp);
        // fclose(fp);

        alBufferData(audio_buffer, AL_BUFFER_FORMAT, audio_buffer_samples, received_samples * sizeof(ALshort), OUT_SAMPLE_RATE);
        NAL_CHECK_ERROR();
        alSourcei(source, AL_BUFFER, audio_buffer);
        NAL_CHECK_ERROR();
        alSourcePlay(source);
        NAL_CHECK_ERROR();
    }

    free(samples_i);
    free(samples_q);

}

int receive_sample_block(hackrf_transfer *transfer) {
    process_sample_block(transfer->buffer, transfer->valid_length / 2);
    return 0;
}

int main(int argc, char **argv) {
    int status;
    hackrf_device *hrf = NULL;

    audio_buffer_samples = calloc(DESIRED_OUT_SAMPLES, sizeof(ALshort)); // We need a bit of extra data because we go over the buffer size.

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
    alGenBuffers(1, &audio_buffer);
    NAL_CHECK_ERROR();

    const int INTER_RATE = 336000;
    const int  MAX_F = 75000;
    const double FILTER = MAX_F * 0.8;

    int filter_freq = FILTER * 0.8;
    downsampler_i = downsampler_new(IN_SAMPLE_RATE, INTER_RATE, filter_freq, 51);
    downsampler_q = downsampler_new(IN_SAMPLE_RATE, INTER_RATE, filter_freq, 51);

    downsampler_audio = downsampler_new(INTER_RATE, OUT_SAMPLE_RATE, 10000, 41);

    // Fill buffer with random data
    //arc4random_buf(data, size);

    // Initialize a source
    alGenSources(1, &source);
    NAL_CHECK_ERROR();

    // Attach buffer to source
    //alSourcei(source, AL_BUFFER, buffer);
    //NAL_CHECK_ERROR();

    // Play
    //alSourcePlay(source);
    //NAL_CHECK_ERROR();

    char *fname = NULL;
    if (argc == 2) {
        fname = argv[1];
    }

    if (fname == NULL) {
        // Live data
        status = hackrf_init();
        HACKRF_CHECK_STATUS(NULL, status, "hackrf_init");

        status = hackrf_open(&hrf);
        HACKRF_CHECK_STATUS(hrf, status, "hackrf_open");

        status = hackrf_set_freq(hrf, CENTER_FREQ);
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

        // Wait 3 seconds to receive data.
        sleep(1);

    } else {
        FILE *fp = fopen(fname, "r");
        fseek(fp, 0L, SEEK_END);
        long size = ftell(fp);
        rewind(fp);
        uint8_t *file_buffer = calloc(size, 1);
        fread(file_buffer, size, 1, fp);
        fclose(fp);
        process_sample_block(file_buffer, size / 2);
    }

    // Playing is asynchronous so wait a while
    sleep(SECONDS);

    // Cleanup
    if (audio_buffer != -1) {
        alDeleteBuffers(1, &audio_buffer);
    }
    alcMakeContextCurrent(NULL);
    alcDestroyContext(ctx);
    alcCloseDevice(device);

    if (fname == NULL) {
        hackrf_stop_rx(hrf);
        hackrf_close(hrf);
        hackrf_exit();
    }
    return 0;
}
