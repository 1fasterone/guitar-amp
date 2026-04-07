# Guitar Amp Modeler — User Guide

## Table of Contents
1. [Getting Started](#getting-started)
2. [Signal Chain Overview](#signal-chain-overview)
3. [Audio Setup](#audio-setup)
4. [Noise Gate](#noise-gate)
5. [Power Metal Distortion](#power-metal-distortion)
6. [Phaser](#phaser)
7. [Tape Echo](#tape-echo)
8. [Preamp](#preamp)
9. [Power Amp](#power-amp)
10. [Cabinet Simulator](#cabinet-simulator)
11. [Reverb](#reverb)
12. [Chromatic Tuner](#chromatic-tuner)
13. [Backing Track Player](#backing-track-player)
14. [Tips & Recipes](#tips--recipes)

---

## Getting Started

**Requirements**
- Windows 10 / 11 (64-bit)
- An audio interface with ASIO or WASAPI drivers (recommended)
- Guitar plugged into the interface input
- Headphones or studio monitors connected to the interface output

**First launch**
1. Run `Guitar Amp.exe` from `build\GuitarAmp_artefacts\Release\`
2. Click **>> AUDIO SETTINGS** in the top-right corner
3. Select your audio interface and sample rate (44100 or 48000 Hz recommended)
4. Set buffer size — lower = less latency (64–256 samples is typical for live playing)
5. Close the dialog and play — signal flows top to bottom through the panel

---

## Signal Chain Overview

Every sample travels through the chain in this exact order:

```
Guitar Input
    │
    ├──► Chromatic Tuner (reads raw signal, no effect on audio)
    │
    ▼
[NOISE GATE]        — silences hum and hiss between notes
    ▼
[POWER METAL DIST]  — high-gain amp distortion stage
    ▼
[GAIN / PREAMP]     — clean preamp level and tone shaping
    ▼
[PHASER]            — modulating all-pass filter effect
    ▼
[TAPE ECHO]         — tape-style delay with wow/flutter
    ▼
[CAB SIM]           — speaker cabinet impulse response
    ▼
[REVERB]            — stereo room / hall ambience
    ▼
[MASTER]            — final output volume
    ▼
Stereo Output  ◄── Backing Track mixed in here
```

---

## Audio Setup

Click **>> AUDIO SETTINGS** at any time to change devices.

| Driver Type | When to Use |
|---|---|
| **ASIO** | Best choice — lowest latency, direct hardware access. Requires an ASIO driver (most audio interfaces include one, or use ASIO4ALL for onboard audio). |
| **WASAPI Exclusive** | Good alternative to ASIO on Windows. Takes exclusive control of the device. |
| **WASAPI Shared** | Allows other apps to use the audio device simultaneously. Slightly higher latency. |
| **DirectSound / WDM** | Last resort. Highest latency, not recommended for live playing. |

**Latency guide**
- **< 5 ms** — imperceptible, ideal for live playing
- **5–10 ms** — acceptable for most players
- **> 15 ms** — noticeable delay, reduce buffer size or switch to ASIO

---

## Noise Gate

Silences the output when your guitar is not being played, eliminating amp hum, cable buzz, and pedal noise between notes and chords.

The gate has four states it cycles through automatically:

```
Signal quiet → CLOSED (muted)
Signal loud  → ATTACK (fading in) → HOLD (fully open) → RELEASE (fading out) → CLOSED
```

| Knob | Range | What It Does |
|---|---|---|
| **THRESHOLD** | −80 to −20 dBFS | The volume level below which the gate closes. Raise it if noise bleeds through between notes; lower it if notes are getting cut off. |
| **ATTACK** | 0.1 – 50 ms | How fast the gate opens when you play. Faster = more percussive attack. Slower = softer fade-in (can sound unnatural). |
| **RELEASE** | 10 – 500 ms | How fast the gate closes after the signal drops. Too fast = choppy cutoff. Too slow = noise tail remains audible. |
| **HOLD** | 0 – 500 ms | Minimum time the gate stays open after the signal goes quiet. Prevents the gate from chattering (rapidly opening and closing) on notes with natural decay. |

**Quick setup:** Set THRESHOLD to around −50 dBFS, play a sustained note, then slowly raise THRESHOLD until hum disappears between notes. Adjust RELEASE so notes end cleanly without a hard chop.

---

## Power Metal Distortion

A high-gain amp distortion modelled on the front-end of a modern metal amp head. Two asymmetric gain stages produce the tight, aggressive sound associated with power metal and djent.

| Knob | Range | What It Does |
|---|---|---|
| **DRIVE** | 0 – 100% | Input gain into the first distortion stage. Low = crunch. High = full saturation and sustain. |
| **TONE** | 0 – 100% | Post-distortion high-shelf filter. Low = dark and warm. High = bright and cutting. |
| **TIGHT** | 0 – 100% | High-pass filter before the distortion. Higher values tighten the low end, reducing mud on low tunings. Equivalent to the "resonance" or "tight" switch on real amp heads. |
| **LEVEL** | 0 – 100% | Output volume of the distortion stage. Use this to match levels with the bypassed signal or to drive the following stages harder. |

**Tip:** Use TIGHT at 30–50% for standard tuning, 60–80% for drop or extended range tunings to prevent bassiness.

---

## Phaser

A four-stage all-pass phaser modelled on the MXR Phase 90. It creates a sweeping, whooshing effect by modulating the phase relationship of frequencies, producing peaks and notches that shift over time.

| Knob | Range | What It Does |
|---|---|---|
| **RATE** | 0.1 – 8 Hz | Speed of the LFO sweep. Low = slow, oceanic movement. High = fast, helicopter-style spinning. |
| **DEPTH** | 0 – 100% | How wide the phase sweep travels. Higher values produce a more pronounced, dramatic effect. |
| **FEEDBACK** | 0 – 80% | Feeds the output back into the input. Increases resonance and makes the sweep more intense and coloured. |
| **MIX** | 0 – 100% | Wet/dry blend. 50% gives the classic phaser sound. 100% is full phase-shifted signal only. |

**Tip:** For subtle thickening on clean tones, keep RATE below 1 Hz and MIX at 40–60%. For classic rock swoosh, try RATE 0.5 Hz, DEPTH 80%, MIX 50%.

---

## Tape Echo

A tape delay emulation with the warmth and instability of a real vintage tape machine. Includes wow/flutter (pitch modulation) and high-frequency loss that characterises magnetic tape playback.

| Knob | Range | What It Does |
|---|---|---|
| **TIME** | 1 – 600 ms | Delay time — the gap between the original signal and the echo. Short times (< 80 ms) produce slapback. Long times (> 200 ms) produce rhythmic echo. |
| **FEEDBACK** | 0 – 85% | How much of the delayed signal is fed back to create repeat echoes. Higher values = more repeats. Capped at 85% to prevent runaway self-oscillation. |
| **MIX** | 0 – 100% | Wet/dry blend. 20–30% is typical for a natural-sounding echo underneath the dry signal. |
| **WOW** | 0 – 100% | Amount of pitch wobble (LFO modulation on delay time). Simulates the speed instability of old tape machines. Adds vintage character and warmth. |

**Tip:** For a U2-style quarter-note delay, set TIME to match your song's tempo (e.g. 500 ms at 120 BPM = quarter note), FEEDBACK at 35%, MIX at 25%.

---

## Preamp

The clean amplifier preamp stage. Controls the overall character and tone of your guitar sound before distortion (if chained before the Distortion) or as the main tone shaper for clean playing.

| Knob | Range | What It Does |
|---|---|---|
| **GAIN** | 0 – 100% | Input drive level. Adds clean volume and harmonic warmth. At higher levels produces gentle tube-style saturation. |
| **BASS** | ±12 dB | Low-frequency shelving filter at 120 Hz. Adds or removes body and thump. |
| **MID** | ±12 dB | Midrange peaking filter at 800 Hz. Cut for a scooped metal sound; boost for bluesy warmth or to cut through a mix. |
| **TREBLE** | ±12 dB | High-frequency shelving filter at 3500 Hz. Controls brightness, pick attack, and string definition. |

**Tip:** The classic "metal scoop" is Bass +4 dB, Mid −4 dB, Treble +3 dB. For blues or classic rock, try Bass 0, Mid +3 dB, Treble 0.

---

## Power Amp

The final amplification stage before the speaker cabinet. Controls the overall loudness and the high-frequency presence that cuts through a mix.

| Knob | Range | What It Does |
|---|---|---|
| **PRESENCE** | ±12 dB | High-shelf filter at 4000 Hz. Adds "air" and definition to the upper harmonics. Equivalent to the Presence knob on a real amp head. Boost to cut through a dense mix; cut for a warmer, smoother tone. |
| **MASTER** | 0 – 100% | Overall output volume. This is your main volume control — it scales after all other processing is complete. |

---

## Cabinet Simulator

Loads an impulse response (IR) of a real guitar speaker cabinet and applies it to your signal via zero-latency convolution. This is the single biggest factor in making a DI guitar signal sound like a mic'd amp.

| Control | What It Does |
|---|---|
| **CAB: OFF / CAB: ON** | Bypasses or enables the cabinet simulation. Toggle to hear the difference. Engages automatically when you first load an IR. |
| **LOAD IR** | Opens a file browser. Select any `.wav` or `.aiff` impulse response file. |
| **BLEND** | 0% = fully dry (no cabinet colouring). 100% = fully processed through the IR. Values between blend the two for a more open sound. |
| **Filename display** | Shows the name of the currently loaded IR file. |

**Where to get free IR files**
- **Celestion** — celestion.com (free IR of their speakers with registration)
- **OwnHammer** — ownhammer.com (free IR packs)
- **God's Cab** — free community IR pack, widely available
- **Rosen Digital** — free modern IR pack

**Supported formats:** WAV and AIFF at any standard sample rate. Stereo files are automatically folded to mono.

**Tip:** A 4×12 Marshall cabinet with Celestion Greenbacks or V30s is the classic choice for distorted rock and metal. A 1×12 open-back with an Alnico Blue gives a classic clean Fender character.

---

## Reverb

A stereo Freeverb-based room/hall reverb. Adds spatial depth and ambience after the cabinet simulation, simulating the acoustic environment where the mic'd cab sits.

| Knob | Range | What It Does |
|---|---|---|
| **MIX** | 0 – 100% | Wet/dry blend. 15–30% is natural-sounding for most applications. Higher values push the sound further back into the room. |
| **SIZE** | 0 – 100% | Room size — controls the length and density of the reverb tail. Low = small room or studio booth. High = large hall or cathedral. |
| **DAMP** | 0 – 100% | High-frequency damping of the reverb tail. Low = bright, reflective surfaces (tile, glass). High = dark, absorptive surfaces (carpet, curtains). |
| **WIDTH** | 0 – 100% | Stereo spread of the wet signal. 0% = mono reverb. 100% = full stereo spread (default). Reduces to mono for mono playback compatibility. |

**Tip:** Keep MIX low (15–20%) for live or band contexts where reverb can muddy the mix. Use SIZE and DAMP together — a large room with high damping sounds natural; a small room with low damping sounds like a live acoustic space.

---

## Chromatic Tuner

Detects the pitch of your guitar string in real time using NSDF (Normalized Square Difference Function) analysis.

| Control | What It Does |
|---|---|
| **TUNER: OFF / TUNER: ON** | Activates pitch detection. When off, no CPU is spent on analysis. |
| **Note display** | Shows the nearest note name and octave (e.g. `E4`, `A3`, `B2`). Turns **green** when within ±8 cents of the target pitch. |
| **Cents display** | Shows how sharp or flat you are (e.g. `+5 cents`, `−12 cents`). |
| **Hz display** | Shows the raw detected frequency in Hz (e.g. `329.6 Hz`). |

**Detection range:** E1 (41 Hz) through E6 (1319 Hz) — covers all six strings in standard, drop D, and most alternate tunings.

**Tuning guide**

| String | Standard | Drop D |
|---|---|---|
| 6 (low) | E2 (82.4 Hz) | D2 (73.4 Hz) |
| 5 | A2 (110.0 Hz) | A2 (110.0 Hz) |
| 4 | D3 (146.8 Hz) | D3 (146.8 Hz) |
| 3 | G3 (196.0 Hz) | G3 (196.0 Hz) |
| 2 | B3 (246.9 Hz) | B3 (246.9 Hz) |
| 1 (high) | E4 (329.6 Hz) | E4 (329.6 Hz) |

**Tip:** Tune with the Noise Gate, Distortion, and Phaser off for the cleanest pitch reading. The tuner reads the raw input signal before any processing, so it works regardless of other settings — but a clean signal detects more reliably.

---

## Backing Track Player

Plays an audio file (MP3, WAV, FLAC, AIFF, OGG) and mixes it directly into the stereo output alongside your processed guitar signal. Use it to practice along to a backing track or click track.

| Control | What It Does |
|---|---|
| **LOAD** | Opens a file browser. Supports MP3, WAV, AIFF, FLAC, and OGG. Files are automatically resampled to match your interface's sample rate. |
| **PLAY / STOP** | Starts or stops playback. If the track has reached the end, pressing PLAY rewinds and starts again. |
| **LOOP: OFF / LOOP: ON** | When on, the track loops seamlessly back to the start when it reaches the end. |
| **VOLUME** | Controls the backing track level independently from the guitar signal. Adjust so the track sits comfortably under your playing. |
| **Filename display** | Shows the name of the currently loaded file. |

**Tip:** Record a drum loop or chord progression in a DAW, export it as a WAV, and load it here to practice improvisation in a specific key and tempo.

---

## Tips & Recipes

### High-Gain Metal (drop tuning)
| Section | Settings |
|---|---|
| Noise Gate | Threshold −45 dBFS, Attack 5 ms, Release 80 ms, Hold 100 ms |
| Distortion | Drive 80%, Tone 55%, Tight 65%, Level 70% |
| Preamp | Gain 50%, Bass +3 dB, Mid −5 dB, Treble +2 dB |
| Power Amp | Presence +3 dB, Master to taste |
| Cab Sim | 4×12 with V30 IR, Blend 100% |
| Reverb | Mix 10%, Size 30%, Damp 60%, Width 100% |

---

### Clean Funk / R&B
| Section | Settings |
|---|---|
| Noise Gate | Threshold −55 dBFS, Attack 2 ms, Release 40 ms, Hold 50 ms |
| Distortion | Drive 15%, Tone 50%, Tight 20%, Level 60% |
| Phaser | Rate 1.2 Hz, Depth 60%, Feedback 30%, Mix 45% |
| Tape Echo | Time 120 ms, Feedback 25%, Mix 20%, Wow 15% |
| Preamp | Gain 40%, Bass +1 dB, Mid +2 dB, Treble +3 dB |
| Power Amp | Presence +1 dB |
| Cab Sim | 1×12 open-back IR, Blend 90% |
| Reverb | Mix 20%, Size 25%, Damp 55%, Width 100% |

---

### Ambient / Post-Rock
| Section | Settings |
|---|---|
| Noise Gate | Threshold −60 dBFS (very light gating) |
| Distortion | Drive 10%, Level 50% (light overdrive for warmth) |
| Phaser | Rate 0.2 Hz, Depth 80%, Feedback 40%, Mix 50% |
| Tape Echo | Time 480 ms, Feedback 65%, Mix 40%, Wow 30% |
| Preamp | Gain 35%, Bass +2 dB, Mid −2 dB, Treble +1 dB |
| Cab Sim | 2×12 IR, Blend 85% |
| Reverb | Mix 45%, Size 80%, Damp 40%, Width 100% |

---

### Vintage Blues
| Section | Settings |
|---|---|
| Noise Gate | Threshold −50 dBFS, Release 150 ms |
| Distortion | Drive 35%, Tone 60%, Tight 15%, Level 65% |
| Tape Echo | Time 220 ms, Feedback 30%, Mix 25%, Wow 40% |
| Preamp | Gain 65%, Bass +2 dB, Mid +4 dB, Treble 0 dB |
| Power Amp | Presence −1 dB |
| Cab Sim | 1×12 alnico IR, Blend 100% |
| Reverb | Mix 25%, Size 35%, Damp 70%, Width 80% |

---

### Reducing Latency

If you experience a noticeable delay between playing and hearing the output:

1. Switch to **ASIO** in Audio Settings if not already using it
2. Reduce the buffer size (try 128 → 64 samples)
3. Close other audio applications competing for the device
4. Disable Windows audio enhancements in Sound Control Panel
5. Make sure your interface drivers are up to date

### Common Issues

**No sound output**
- Check Audio Settings — confirm the correct output device is selected
- Make sure "Listen to this device" is disabled in Windows Sound settings for your input (it routes the signal twice, bypassing the app)

**Tuner shows no reading**
- Enable the tuner with the TUNER: ON button
- Play one string cleanly — the tuner needs at least one strong pitch to lock onto
- Mute the Distortion (low Drive) for cleaner pitch detection

**Backing track sounds at wrong pitch**
- The backing track is resampled automatically — this is normal
- Ensure your interface sample rate matches the track's sample rate (both 44100 or both 48000 Hz)

**Cab Sim sounds harsh without an IR loaded**
- Leave CAB: OFF until an IR is loaded — running convolution with no IR produces silence or an impulse peak
- Load a `.wav` IR file first, then enable
