# Siren

*If there's a speaker, Siren will play sound.*

```clojure
(load "siren/siren.carp")

(defn main []
  (let-do [mix (Mixer.make 2)]
    (Mixer.set-channel! &mix 0 (lead-triangle 0.5f))
    (Mixer.set-seq!     &mix 0 (Seq.make [(note note-c4 200)
                                          (note note-e4 200)
                                          (note note-g4 400)]))
    (Mixer.set-channel! &mix 1 (bass-saw 0.4f))
    (Mixer.set-seq!     &mix 1 (Seq.make [(note note-c3 800)]))
    (Siren.init)
    (Siren.run &mix)))
```

That's it. Define instruments, define melodies, play them together. Anywhere.

## What It Is

Siren is a chiptune audio engine written in [Carp](https://github.com/carp-lang/Carp). It compiles to C and depends only on SDL2 — the same library that powers Carp's game ecosystem. No runtime. No garbage collector. No proprietary dependencies. No license fees.

SDL2 runs on everything worth running on: desktop, Raspberry Pi, embedded ARM, handheld hardware, art installations, homebrew game consoles. If someone cared enough to hook up a speaker, SDL2 probably already runs there. So does Siren.

This is the point. Not a feature list — just this: **if there's a speaker, Siren will play sound.** Minimal code, any hardware, real music.

## Why It Exists

Most audio engines are either:
- **Massive frameworks** (FMOD, Wwise) — proprietary, expensive, overkill
- **Runtime-dependent** (Python, JS) — not truly embeddable
- **Engine-locked** (Unity, Godot audio) — useless outside their host

Siren is none of those. It's a library. You own it completely.

It was built as part of an ongoing effort to make [Carp](https://github.com/carp-lang/Carp) viable as a general purpose systems language — proving that a compiled Lisp with no GC can power real audio DSP cleanly and ergonomically.

## The Pipeline

```
Sequencer → Oscillator → Envelope → [Filter] → Mixer → SDL2 → PCM out
```

Every stage is a pure transform on f32 arrays. Data flows one direction. Ownership is linear. Nothing is shared between stages.

## What You Get

- **5 oscillators** — sine, square, saw, triangle, noise
- **ADSR envelopes** — full state machine, retrigger-safe
- **Biquad filters** — low-pass, high-pass, band-pass per channel
- **LFO** — pitch vibrato and amplitude tremolo
- **Step sequencer** — note arrays with rests, loops continuously
- **Polyphonic mixer** — N named voices + stereo pan + output clamp
- **Fire-and-forget SFX** — `(Mixer.sfx! &mix freq instrument)`, no slot management
- **WAV playback** — mono 16-bit WAV files, one-shot or looping
- **Preset instruments** — `(pluck-square 0.4f)`, `(pad-sine 0.3f)`, `(bass-saw 0.5f)`
- **88 named notes** — `note-c4` through `note-b7`, all piano keys, equal temperament
- **BPM helpers** — `beats->ms`, `defbpm` macro for tempo-synced durations
- **One-line startup** — `Siren.run` owns the engine loop

## Dependencies

- [Carp](https://github.com/carp-lang/Carp) — the compiler
- [SDL2](https://www.libsdl.org/) — audio output (already required by Carp game projects)

## Getting Started

Clone Siren into your project directory:

```bash
git clone https://github.com/sqrew/siren.git
```

Load it from your Carp source:

```clojure
(load "siren/siren.carp")
```

Install SDL2 if you haven't already:

```bash
# Arch / Manjaro
sudo pacman -S sdl2

# Debian / Ubuntu
sudo apt install libsdl2-dev

# macOS
brew install sdl2
```

Run the example:

```bash
carp -x examples/basic.carp
```

Run the tests:

```bash
carp test/run.carp -x
```

Full API documentation: [DOCS.md](./DOCS.md)

## Status

Functional and tested. 103 tests, all passing. Real polyphonic music plays. The architecture is proven. Not production-hardened — this is a research project and a love letter to a language that deserves more users.

## Built With

sqrew + Claude (claude-sonnet-4-6) — southern Maine, 2026.
