#ifndef SIREN_AUDIO_H
#define SIREN_AUDIO_H

#include <SDL2/SDL.h>

/* Open the default audio device in queue mode (no callback).
   Returns device ID as int, or 0 on failure. */
static int __attribute__((unused)) siren_open_audio(int freq, int channels, int samples) {
    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq     = freq;
    spec.format   = AUDIO_F32SYS;
    spec.channels = (Uint8)channels;
    spec.samples  = (Uint16)samples;
    spec.callback = NULL;
    return (int)SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
}

/* Pause or unpause device. pause_on: 1=pause, 0=play */
static void __attribute__((unused)) siren_pause(int dev, int pause_on) {
    SDL_PauseAudioDevice((SDL_AudioDeviceID)dev, pause_on);
}

/* Queue a float buffer for playback. len is number of floats. */
static int __attribute__((unused)) siren_queue_audio(int dev, float* buf, int len) {
    return SDL_QueueAudio((SDL_AudioDeviceID)dev, buf,
                          (Uint32)(len * sizeof(float)));
}

/* Bytes currently queued on device. */
static int __attribute__((unused)) siren_queued_bytes(int dev) {
    return (int)SDL_GetQueuedAudioSize((SDL_AudioDeviceID)dev);
}

/* Close a device. */
static void __attribute__((unused)) siren_close(int dev) {
    SDL_CloseAudioDevice((SDL_AudioDeviceID)dev);
}

/* Load a mono 16-bit PCM WAV at 44100 Hz into a normalized f32 array.
   Returns an empty array on any error or format mismatch.
   The returned data is malloc'd — Carp's Array destructor will free it.
   Return type is Array, which Carp defines in carp_memory.h before this header. */
static Array __attribute__((unused)) siren_load_wav(const char *path) {
    Array result = {0, 0, NULL};  /* len, capacity, data */
    SDL_AudioSpec spec;
    Uint8 *sdl_buf = NULL;
    Uint32 sdl_len = 0;

    if (!SDL_LoadWAV(path, &spec, &sdl_buf, &sdl_len)) return result;

    if (spec.freq != 44100 || spec.channels != 1 || spec.format != AUDIO_S16LSB) {
        SDL_FreeWAV(sdl_buf);
        return result;
    }

    int n = (int)(sdl_len / sizeof(Sint16));
    float *floats = (float *)malloc((size_t)n * sizeof(float));
    if (!floats) { SDL_FreeWAV(sdl_buf); return result; }

    Sint16 *src = (Sint16 *)sdl_buf;
    for (int i = 0; i < n; i++) floats[i] = src[i] / 32768.0f;
    SDL_FreeWAV(sdl_buf);

    result.len      = (size_t)n;
    result.capacity = (size_t)n;
    result.data     = floats;
    return result;
}


#endif
