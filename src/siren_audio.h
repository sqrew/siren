#ifndef SIREN_AUDIO_H
#define SIREN_AUDIO_H

#include <SDL2/SDL.h>

/* Open the default audio device in queue mode (no callback).
   Returns device ID, or 0 on failure. */
static SDL_AudioDeviceID siren_open_audio(int freq, int channels, int samples) {
    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq     = freq;
    spec.format   = AUDIO_F32SYS;
    spec.channels = (Uint8)channels;
    spec.samples  = (Uint16)samples;
    spec.callback = NULL;
    return SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
}

/* Queue a float buffer for playback.
   len is number of floats (not bytes). */
static int siren_queue_audio(SDL_AudioDeviceID dev, float* buf, int len) {
    return SDL_QueueAudio(dev, buf, (Uint32)(len * sizeof(float)));
}

/* Bytes currently queued on device. */
static int siren_queued_bytes(SDL_AudioDeviceID dev) {
    return (int)SDL_GetQueuedAudioSize(dev);
}

#endif
