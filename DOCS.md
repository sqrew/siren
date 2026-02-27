# Siren Documentation

*If there's a speaker, Siren will play sound.*

## Overview

Siren is a chiptune and procedural audio engine written in [Carp](https://github.com/carp-lang/Carp). It compiles to C, depends only on SDL2, and runs on anything SDL2 runs on: desktop, Raspberry Pi, embedded ARM, handhelds, art installations.

Siren generates audio from scratch — oscillators, envelopes, filters, sequencers — and mixes it down to stereo PCM that gets pushed to SDL2's audio queue. It can also play back WAV files. No codecs, no external asset pipeline, no runtime.

### Architecture

```
Sequencer --> Oscillator --+
                           +--> Envelope --> Filter --> Mixer --> SDL2 audio queue --> PCM out
                    LFO ---+
```

All stages are pure transforms on `f32` arrays. Data flows one direction. Ownership is linear — nothing is shared between stages. Everything is mono internally; the Mixer handles stereo panning and summing at the output boundary.

### What You Get

- **5 oscillators** — sine, square, saw, triangle, noise
- **ADSR envelopes** — full state machine, retrigger-safe
- **Biquad filters** — low-pass, high-pass, band-pass per channel
- **LFO** — pitch vibrato and amplitude tremolo
- **Step sequencer** — note arrays with rests, loops continuously
- **Polyphonic mixer** — N named voices + stereo pan + output clamp
- **Fire-and-forget SFX** — `(Mixer.sfx! &mix freq instrument)`, no slot management
- **WAV playback** — load and play mono 16-bit WAV files, one-shot or looping
- **Preset instruments** — `(pluck-square 0.4f)`, `(pad-sine 0.3f)`, `(bass-saw 0.5f)`
- **88 named notes** — `note-c4` through `note-b7`, all piano keys, equal temperament
- **BPM helpers** — `beats->ms`, `defbpm` macro for tempo-synced durations
- **One-line startup** — `Siren.run` owns the engine loop

---

## Installation

### Dependencies

- **Carp** — the compiler ([github.com/carp-lang/Carp](https://github.com/carp-lang/Carp))
- **SDL2** — audio output (already required by most Carp game projects)

Install SDL2 via your system package manager:

```bash
# Arch / Manjaro
sudo pacman -S sdl2

# Debian / Ubuntu
sudo apt install libsdl2-dev

# macOS (homebrew)
brew install sdl2
```

### Loading Siren

Clone Siren into your project directory, then load the single entry point:

```clojure
(load "siren/siren.carp")
```

This loads everything: constants, notes, oscillators, envelope, filter, LFO, channel, sequencer, BPM helpers, WAV playback, presets, mixer, and the Siren runtime.

### Running Tests

```bash
carp test/run.carp -x
```

103 tests, 0 failures.

---

## Core Concepts

### Mixer

The top-level container. Owns N named voice slots (each a Channel + Seq pair), a 4-channel SFX pool for fire-and-forget sounds, and a 4-slot WAV sample player pool. Handles stereo panning, summing, master volume, and output clamping.

```clojure
(let [mix (Mixer.make 3)]  ; 3 named voice slots
  ...)
```

Each tick, the Mixer:
1. Zeros the stereo output buffer
2. Ticks all Seqs (which drive their paired Channels)
3. Ticks all Channels (osc → envelope → filter → gain)
4. Ticks all SFX pool channels
5. Ticks all SamplePlayers
6. Sums everything to interleaved stereo with per-voice panning
7. Applies master volume, clamps to -1.0..1.0

### Channel

A complete synthesizer voice: oscillator + ADSR envelope + biquad filter + LFO. Define an instrument once with `Channel.make`, then trigger notes with `play!` and `release!`.

```clojure
; (Channel.make waveform attack-ms decay-ms sustain release-ms gain)
(let [lead (Channel.make osc-triangle 20 80 0.8f 150 0.5f)]
  ...)
```

The `waveform` parameter is one of: `osc-sine` (0), `osc-square` (1), `osc-saw` (2), `osc-triangle` (3), `osc-noise` (4).

Filter and LFO are off by default — zero overhead when unused. Enable them with `set-lp!`/`set-hp!`/`set-bp!` and `set-lfo-freq!`/`set-lfo-amp!`.

### Seq (Sequencer)

A monophonic step sequencer. Walks an array of `Note` values, firing `Channel.play!` and `Channel.release!` at the correct sample-accurate times. Loops continuously.

```clojure
(let [melody (Seq.make [(note note-c4 500)
                        (note note-e4 500)
                        (note note-g4 1000)])]
  ...)
```

### SamplePlayer

Plays back raw PCM audio loaded from WAV files. Supports one-shot (plays once, deactivates) and looping (wraps back to a loop point) modes.

### Env (Envelope)

ADSR amplitude envelope with a full state machine: attack → decay → sustain → release → done. Times are in milliseconds. Retrigger-safe — calling `note-on!` during any stage restarts from attack. Release fades from wherever the envelope actually is when `note-off!` fires, not from 1.0.

```
    1.0 ___
       /    \
      /      \_____ sustain
     /              \
    /                \___  0.0
   A    D    S     R
```

### Filter

Biquad filter with three modes: low-pass, high-pass, band-pass. Coefficients from the Audio EQ Cookbook. The `q` parameter controls resonance: 0.707 = Butterworth (maximally flat), >1.0 = resonant peak at cutoff.

### Lfo

Sine-wave modulation with two targets: **lfo-freq** (pitch vibrato, ±depth Hz) and **lfo-amp** (tremolo, scales amplitude). Resolution is one value per audio buffer (~86 updates/sec at 44100/512).

### Osc (Oscillators)

Five signal generators producing mono f32 buffers. Phase is passed in and returned — the caller owns state, waveforms stay continuous across buffer boundaries.

- **sine** — classic, band-limited by nature
- **square** — naive (aliased), good for chiptune
- **saw** — naive (aliased), good for chiptune
- **triangle** — derived from saw, quasi-band-limited
- **noise** — white noise

---

## Quick Start

```clojure
(load "siren/siren.carp")

(defn main []
  (let-do [mix (Mixer.make 1)]
    (Mixer.set-channel! &mix 0 (lead-triangle 0.5f))
    (Mixer.set-seq! &mix 0 (Seq.make [(note note-c4 500)
                                      (note note-e4 500)
                                      (note note-g4 1000)]))
    (Siren.init)
    (Siren.run &mix)))
```

You should hear a looping C major arpeggio. Press Ctrl-C to stop.

### Embedding in a Game Loop

```clojure
(defn game-main []
  (let-do [mix (Mixer.make 3)
           _   (Siren.init)
           dev (Siren.open)]
    (while game-running
      (do
        ; your game logic here
        (Siren.tick &dev &mix)))))
```

`Siren.tick` queues enough buffers to stay ahead of the audio device (~0.1ms, safe at 30/60/120 fps).

---

## API Reference

### Siren (Runtime)

| Function | Signature | Description |
|----------|-----------|-------------|
| `Siren.init` | `() -> ()` | Initialize SDL audio subsystem |
| `Siren.open` | `() -> Int` | Open audio device and start playback; returns device ID |
| `Siren.tick` | `(Ref Int) (Ref Mixer) -> ()` | Pump audio for one frame |
| `Siren.run` | `(Ref Mixer) -> ()` | Blocking standalone loop; opens device and runs forever |

### Mixer

| Function | Signature | Description |
|----------|-----------|-------------|
| `Mixer.make` | `Int -> Mixer` | Create mixer with N named voice slots |
| `Mixer.set-channel!` | `(Ref Mixer) Int Channel -> ()` | Replace the Channel in named slot i |
| `Mixer.set-seq!` | `(Ref Mixer) Int Seq -> ()` | Replace the Seq in named slot i |
| `Mixer.set-pan!` | `(Ref Mixer) Int Float -> ()` | Set pan for slot i (-1.0 left, 0.0 center, 1.0 right) |
| `Mixer.sfx!` | `(Ref Mixer) Float Channel -> ()` | Fire a one-shot SFX at center pan |
| `Mixer.sfx-at!` | `(Ref Mixer) Float Float Channel -> ()` | Fire a one-shot SFX at explicit pan |
| `Mixer.set-master-vol!` | `(Ref Mixer) Float -> ()` | Set master volume (0.0–1.0, default 1.0) |
| `Mixer.set-sample-player!` | `(Ref Mixer) Int SamplePlayer -> ()` | Install a SamplePlayer in sample slot i |
| `Mixer.set-sample-pan!` | `(Ref Mixer) Int Float -> ()` | Set pan for sample slot i |
| `Mixer.play-sample!` | `(Ref Mixer) Int -> ()` | Start playback on sample slot i |
| `Mixer.stop-sample!` | `(Ref Mixer) Int -> ()` | Stop playback on sample slot i |
| `Mixer.tick` | `(Ref Mixer) -> ()` | Tick all voices, sum to stereo, clamp output |

### Channel

| Function | Signature | Description |
|----------|-----------|-------------|
| `Channel.make` | `Int Int Int Float Int Float -> Channel` | waveform, attack-ms, decay-ms, sustain, release-ms, gain |
| `Channel.play!` | `(Ref Channel) Float -> ()` | Trigger a note at the given frequency (Hz) |
| `Channel.release!` | `(Ref Channel) -> ()` | Release the current note |
| `Channel.set-lp!` | `(Ref Channel) Float Float -> ()` | Enable low-pass filter: cutoff Hz, Q |
| `Channel.set-hp!` | `(Ref Channel) Float Float -> ()` | Enable high-pass filter: cutoff Hz, Q |
| `Channel.set-bp!` | `(Ref Channel) Float Float -> ()` | Enable band-pass filter: center Hz, Q |
| `Channel.clear-filter!` | `(Ref Channel) -> ()` | Disable filter |
| `Channel.set-lfo-freq!` | `(Ref Channel) Float Float -> ()` | Enable pitch vibrato: rate Hz, depth Hz |
| `Channel.set-lfo-amp!` | `(Ref Channel) Float Float -> ()` | Enable tremolo: rate Hz, depth 0..1 |
| `Channel.clear-lfo!` | `(Ref Channel) -> ()` | Disable LFO |

### Seq / Note

| Function | Signature | Description |
|----------|-----------|-------------|
| `Seq.make` | `(Array Note) -> Seq` | Create a sequencer from a note array |
| `Note.make` | `Float Int -> Note` | frequency Hz, duration ms |
| `Note.rest` | `Int -> Note` | Silence of given duration ms |
| `note` | macro | Sugar: `(note note-c4 500)` → `(Note.make note-c4 500)` |
| `defsong` | macro | Build a typed `(Array Note)` from variadic notes |

### Filter

| Function | Signature | Description |
|----------|-----------|-------------|
| `Filter.make-lp` | `Float Float -> Filter` | Low-pass: cutoff Hz, Q |
| `Filter.make-hp` | `Float Float -> Filter` | High-pass: cutoff Hz, Q |
| `Filter.make-bp` | `Float Float -> Filter` | Band-pass: center Hz, Q |
| `Filter.fill` | `(Ref (Array Float)) (Ref Filter) -> ()` | Apply filter in-place to a mono buffer |

### Osc

All oscillators: `(Ref (Array Float)) Float Float -> Float` — buffer, frequency Hz, phase → next phase.

| Function | Description |
|----------|-------------|
| `Osc.fill-sine` | Sine wave |
| `Osc.fill-square` | Square wave |
| `Osc.fill-saw` | Sawtooth wave |
| `Osc.fill-triangle` | Triangle wave |
| `Osc.fill-noise` | White noise |

### SamplePlayer / Wav

| Function | Signature | Description |
|----------|-----------|-------------|
| `SamplePlayer.make` | `(Array Float) -> SamplePlayer` | One-shot player |
| `SamplePlayer.make-loop` | `(Array Float) Int Int -> SamplePlayer` | Looping player: samples, loop-start, loop-end |
| `SamplePlayer.play!` | `(Ref SamplePlayer) -> ()` | Start/restart playback |
| `SamplePlayer.stop!` | `(Ref SamplePlayer) -> ()` | Stop playback |
| `Wav.load` | `(Ref String) -> (Array Float)` | Load mono 16-bit 44100 Hz WAV as f32 samples |

> **Note:** `Wav.load` only supports mono, 16-bit, 44100 Hz WAV files. Anything else returns an empty array silently — check `(Array.length &samples)`.

### BPM Helpers

| Function/Macro | Description |
|----------------|-------------|
| `beats->ms` | `Int Float -> Int` — BPM + beat count → milliseconds |
| `defbpm` | Defines `whole`, `half`, `quarter`, `eighth`, `sixteenth`, `dotted-*`, `triplet-*` at given BPM |

Beat values: 1.0 = quarter, 2.0 = half, 4.0 = whole, 0.5 = eighth, 0.25 = sixteenth.

### Constants

| Name | Value | Description |
|------|-------|-------------|
| `sample-rate` | 44100 | Samples per second |
| `buf-frames` | 512 | Mono frames per buffer (~11.6ms) |
| `buf-size` | 1024 | Floats per stereo buffer |
| `osc-sine` | 0 | Sine waveform constant |
| `osc-square` | 1 | Square waveform constant |
| `osc-saw` | 2 | Saw waveform constant |
| `osc-triangle` | 3 | Triangle waveform constant |
| `osc-noise` | 4 | Noise waveform constant |

---

## Presets

Presets are macros expanding to `Channel.make` with baked-in ADSR. Pass a gain value (0.0–1.0).

| Preset | Character | Best For |
|--------|-----------|----------|
| `pluck-square`, `pluck-saw` | Fast attack, percussive | Leads, SFX, staccato |
| `pad-sine`, `pad-triangle` | Slow attack, airy sustain | Chords, atmosphere |
| `bass-saw`, `bass-square` | Punchy, short decay | Bass lines |
| `lead-square`, `lead-triangle` | Medium attack, singing sustain | Melodies |
| `perc-noise`, `perc-square` | Instant attack, drum-like | Drums, percussion |

Custom instrument:

```clojure
; saw wave, 10ms attack, 200ms decay, 0.3 sustain, 500ms release, 0.6 gain
(Channel.make osc-saw 10 200 0.3f 500 0.6f)
```

---

## Notes Reference

All 88 piano keys defined as `Float` Hz constants using equal temperament (`440 * 2^((n-49)/12)`). Sharps use `s` suffix: `note-cs4` = C#4.

Range: `note-a0` (27.50 Hz) through `note-c8` (4186.01 Hz). Key reference points:

| Note | Hz | Note | Hz |
|------|----|------|----|
| `note-c4` | 261.63 | `note-a4` | 440.00 |
| `note-c5` | 523.25 | `note-a5` | 880.00 |
| `note-c3` | 130.81 | `note-a3` | 220.00 |

Any `Float` Hz value works — not limited to named constants.

---

## SFX

```clojure
; center-panned one-shot
(Mixer.sfx! &mix note-c5 (pluck-square 0.5f))

; positional
(Mixer.sfx-at! &mix note-a3 -0.8f (perc-noise 0.4f))  ; mostly left

; from game logic
(when player-jumped  (Mixer.sfx! &mix note-c5 (pluck-square 0.5f)))
(when enemy-hit      (Mixer.sfx! &mix note-a3 (perc-noise 0.4f)))
```

The SFX pool has 4 round-robin slots. Use presets with `sustain = 0.0` (pluck, perc, bass) for clean one-shots.

---

## WAV Playback

```clojure
; one-shot
(let [samples (Wav.load "assets/kick.wav")]
  (Mixer.set-sample-player! &mix 0 (SamplePlayer.make samples))
  (Mixer.play-sample! &mix 0))

; looping
(let [pad (Wav.load "assets/ambient.wav")]
  (Mixer.set-sample-player! &mix 1
    (SamplePlayer.make-loop pad 0 (Array.length &pad)))
  (Mixer.play-sample! &mix 1))
```

---

## Source Files

| File | Purpose |
|------|---------|
| `src/siren.carp` | Entry point; loads everything; defines Siren runtime |
| `src/constants.carp` | Engine constants, `ms-to-samps` |
| `src/notes.carp` | 88 piano key frequency constants |
| `src/oscillators.carp` | 5 oscillator fill functions |
| `src/envelope.carp` | ADSR envelope state machine |
| `src/filter.carp` | Biquad filter: LP/HP/BP |
| `src/lfo.carp` | LFO for pitch/amplitude modulation |
| `src/channel.carp` | Complete voice: osc + env + filter + LFO |
| `src/sequencer.carp` | Step sequencer and Note type |
| `src/bpm.carp` | `beats->ms` and `defbpm` macro |
| `src/wav.carp` | WAV sample player |
| `src/presets.carp` | Instrument presets, `note`/`defsong` macros |
| `src/mixer.carp` | Polyphonic mixer with stereo output |
| `src/SDLAudio.carp` | SDL2 audio queue bindings |
| `examples/basic.carp` | Comprehensive example |
| `test/run.carp` | Test runner (103 tests) |
