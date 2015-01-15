// Play random noise as audio

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <OpenAL/al.h>
#include <OpenAL/alc.h>

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
        exit(1);
    }
}

#define NAL_CHECK_ERROR() nal_check_error(__FILE__, __LINE__)

int main() {
    // Initialize the audio context
    ALCdevice *device = alcOpenDevice(NULL);
    if (!device) {
        fprintf(stderr, "Could not open audio device.\n");
        return 1;
    }
    ALCcontext *ctx = alcCreateContext(device, NULL);
    alcMakeContextCurrent(ctx);

    // Initialize an audio buffer
    ALuint buffer;
    alGetError(); // clear error code
    alGenBuffers(1, &buffer);
    NAL_CHECK_ERROR();

    // Fill buffer with random data
    ALenum format = AL_FORMAT_MONO8;
    ALsizei size = 44100;
    ALsizei freq = 44100;

    unsigned char *data[size];
    arc4random_buf(data, size);

    alBufferData(buffer, format, data, size, freq);

    // Initialize a source
    ALuint source;
    alGenSources(1, &source);
    NAL_CHECK_ERROR();

    // Attach buffer to source
    alSourcei(source, AL_BUFFER, buffer);
    NAL_CHECK_ERROR();

    // Play
    alSourcePlay(source);
    NAL_CHECK_ERROR();

    // Playing is asynchronous so wait a while
    sleep(1);

    // Cleanup
    alDeleteBuffers(1, &buffer);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(ctx);
    alcCloseDevice(device);

    return 0;
}
