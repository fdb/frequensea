// Play random noise as audio

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <OpenAL/al.h>
#include <OpenAL/alc.h>

#include <libhackrf/hackrf.h>

#define HACKRF_CHECK_STATUS(device, status, message) \
if (status != 0) { \
    fprintf(stderr, "HackRF error: %s\n", message); \
    hackrf_close(device); \
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

int receive_sample_block(hackrf_transfer *transfer) {
    if (received_size > size) return 0;


    for (int i = 0; i < transfer->valid_length; i += 200) {
        float ii = transfer->buffer[i];
        float qq = transfer->buffer[i+1];
        ALuint mag = sqrt(ii * ii + qq * qq) * 1;
        data[received_size++] = mag ;
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
    HACKRF_CHECK_STATUS(hrf, status, "hackrf_init");

    status = hackrf_open(&hrf);
    HACKRF_CHECK_STATUS(hrf, status, "hackrf_open");

    status = hackrf_set_freq(hrf, 124.20005e6);
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
