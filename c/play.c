// Play random noise as audio

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <sndfile.h>

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

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: play <audio_file.wav>\n");
        exit(1);
    }
    const char *file_name = argv[1];

    // Initialize the audio context
    ALCdevice *device = alcOpenDevice(NULL);
    if (!device) {
        fprintf(stderr, "Could not open audio device.\n");
        return 1;
    }
    ALCcontext *ctx = alcCreateContext(device, NULL);
    alcMakeContextCurrent(ctx);

    //alBufferData(buffer, format, data, size, freq);
    SF_INFO wav_file_info;
    SNDFILE* wav_file = sf_open(file_name, SFM_READ, &wav_file_info);

    ALsizei n_samples  = wav_file_info.channels * wav_file_info.frames;
    ALsizei sample_rate = wav_file_info.samplerate;
    int16_t *samples = calloc(n_samples, sizeof(short));
    if (sf_read_short(wav_file, samples, n_samples) < n_samples) {
        fprintf(stderr, "Error while reading audio.\n");
        exit(1);
    }

    sf_close(wav_file);

    // Initialize an audio buffer
    ALuint buffer;
    alGetError(); // clear error code
    alGenBuffers(1, &buffer);
    NAL_CHECK_ERROR();

    alBufferData(buffer, AL_FORMAT_MONO16, samples, n_samples * sizeof(ALushort), sample_rate);
    NAL_CHECK_ERROR();

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
    sleep(3);

    // Cleanup
    alDeleteBuffers(1, &buffer);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(ctx);
    alcCloseDevice(device);

    return 0;
}
