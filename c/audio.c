// Play random noise as audio

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <OpenAL/al.h>
#include <OpenAL/alc.h>

#include <libhackrf/hackrf.h>


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
const ALsizei freq = 48000;
const ALsizei size = freq * 5;
ALuint source;

ALuint *data;
int has_received = 0;

int received_size = 0;
vec2 *vec_buffer;

#define HACKRF_SAMPLES_SIZE 131072
#define HACKRF_BUFFER_SIZE 262144


int receive_sample_block(hackrf_transfer *transfer) {
    if (received_size > size) return 0;

    // Convert to vec2
    for (int i = 0; i < HACKRF_SAMPLES_SIZE; i++) {
        unsigned int vi = transfer->buffer[i * 2];
        unsigned int vq = transfer->buffer[i * 2 + 1];
        vec_buffer[i].x = vi; //(vi - 128.0) / 256.0;
        vec_buffer[i].y = vq; // (vq - 128.0) / 256.0;
    }

    // Moving average
    int decimation_rate = 100;

    for (int i = 0; i < HACKRF_SAMPLES_SIZE; i += decimation_rate) {
        float sum_i = 0;
        float sum_q = 0;
        for (int j = 0; j < decimation_rate; j++) {
            sum_i += vec_buffer[i + j].x;
            sum_q += vec_buffer[i + j].y;
        }
        float avg_i = sum_i / (float) decimation_rate;
        float avg_q = sum_q / (float) decimation_rate;
        ALuint mag = sqrt(avg_i * avg_i + avg_q * avg_q) * 10;
        data[received_size++] = mag;
    }

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

    status = hackrf_set_freq(hrf, 124.2001e6);
    HACKRF_CHECK_STATUS(hrf, status, "hackrf_set_freq");

    status = hackrf_set_sample_rate(hrf, 5e6);
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
    sleep(10);

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
