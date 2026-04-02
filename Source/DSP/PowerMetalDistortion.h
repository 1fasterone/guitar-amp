#pragma once
#include "BiquadFilter.h"
#include <cmath>
#include <algorithm>

// Power Metal Distortion — two-stage asymmetric clipper pedal.
//
// Signal chain:
//   Input
//     │ [TightHPF]     1st-order HPF 60–180 Hz   (removes pre-clip mud)
//     │ [BiteBoost]    Peak +0–9 dB @ 2.8 kHz    (pick attack / chunk)
//     │ [Stage 1]      Symmetric tanh clip        (warm tube-like saturation)
//     │ [InterHPF]     1st-order HPF @ 90 Hz      (inter-stage DC/sub removal)
//     │ [Stage 2]      Asymmetric clip             (aggression + 2nd harmonic)
//     │ [DCBlocker]    1-pole IIR @ 10 Hz         (clears residual DC offset)
//     │ [ToneShelf]    High-shelf ±9 dB @ 3 kHz   (dark ↔ bright post-clip)
//     │ [Level]        Square-law output scalar
//   Output → GainStage → ToneStack → Presence → Master
//
// Parameter ranges (all 0.0 – 1.0):
//   Drive  0 = light edge, 1 = full power metal saturation
//   Tone   0 = dark/heavy,  0.5 = flat,  1 = bright/cutting
//   Tight  0 = loose,       1 = hyper-tight (djent-adjacent)
//   Level  0 = silence,     1 = full output
class PowerMetalDistortion
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        // DC blocker pole: R = 1 - (2π * 10 Hz / fs)
        // At 44100 Hz → R ≈ 0.99857, at 48000 Hz → R ≈ 0.99869
        static constexpr double pi = 3.14159265358979323846;
        dcR  = 1.0 - (2.0 * pi * 10.0 / sr);
        dcX1 = dcY1 = 0.0;
        // Pre-compute the fixed inter-stage HPF (never changes)
        interHPF.setCoeffs(makeFirstOrderHPF(90.0, sr));
        // Rebuild variable filters with current parameter values
        rebuild();
    }

    // ---- Parameter setters (call from audio thread only) --------------------

    void setDrive(float v) noexcept { drive = v; rebuild(); }
    void setTone (float v) noexcept { tone  = v; rebuildTone(); }
    void setTight(float v) noexcept { tight = v; rebuildTight(); }
    void setLevel(float v) noexcept { level = v; }

    // ---- Per-sample processing (audio thread only) --------------------------

    double process(double x) noexcept
    {
        // 1. Tightness high-pass (remove pre-clip low-end mud)
        x = tightHPF.process(x);

        // 2. Bite boost (emphasise pick-attack frequencies before clipping)
        x = biteBoost.process(x);

        // 3. Stage 1 — symmetric soft clip (tanh)
        //    gain 3x (clean edge) → 20x (full saturation)
        const double g1 = 3.0 + drive * 17.0;
        x = std::tanh(g1 * x);

        // 4. Inter-stage HPF — strips sub-bass buildup between stages
        x = interHPF.process(x);

        // 5. Stage 2 — asymmetric hard/soft clip
        //    Positive half : smooth compression (tube-like pick attack)
        //    Negative half : harder floor (transistor / diode aggression)
        const double g2 = 2.0 + drive * 6.0;
        x = asymClip(x * g2);

        // 6. DC blocker — single-pole IIR @ 10 Hz
        //    y[n] = x[n] - x[n-1] + R * y[n-1]
        const double dcOut = x - dcX1 + dcR * dcY1;
        dcX1 = x;
        dcY1 = dcOut;
        x = dcOut;

        // 7. Tone shelf — shapes post-clip harmonic content
        x = toneShelf.process(x);

        // 8. Output level (square law for natural taper)
        return x * (double)(level * level);
    }

private:
    // ---- DSP helpers --------------------------------------------------------

    // Asymmetric waveshaper:
    //   Positive: tanh → soft ceiling ~1.0  (smooth, sustain-rich)
    //   Negative: linear to -0.80, then tanh shoulder to ~-0.975 (firm, aggressive)
    static double asymClip(double x) noexcept
    {
        if (x >= 0.0)
        {
            return std::tanh(x);
        }
        else
        {
            constexpr double threshold = -0.80;
            if (x > threshold)
                return x;                   // linear — preserves pick transient
            else
                return threshold
                       - std::tanh(-(x - threshold) * 0.5) * 0.35;
                       // soft shoulder → hard limit ≈ -0.975
        }
    }

    // 1st-order HPF as a degenerate biquad (b2=a2=0).
    // H(z) = (1 - z⁻¹) / ((1+K) - (1-K)z⁻¹)   where K = tan(π·fc/fs)
    static BiquadFilter::Coeffs makeFirstOrderHPF(double fc, double sampleRate)
    {
        static constexpr double pi = 3.14159265358979323846;
        const double K    = std::tan(pi * fc / sampleRate);
        const double norm = 1.0 / (1.0 + K);
        return { norm, -norm, 0.0, (K - 1.0) * norm, 0.0 };
    }

    // ---- Filter rebuild helpers ---------------------------------------------

    void rebuild()
    {
        if (sr <= 0.0) return;
        rebuildTight();
        rebuildBite();
        rebuildTone();
    }

    void rebuildTight()
    {
        if (sr <= 0.0) return;
        // Tight knob: 60 Hz (loose, only cuts sub-rumble)
        //           → 180 Hz (hyper-tight, cuts into low-mid body)
        const double fc = 60.0 + tight * 120.0;
        tightHPF.setCoeffs(makeFirstOrderHPF(fc, sr));
    }

    void rebuildBite()
    {
        if (sr <= 0.0) return;
        // Drive scales the bite-boost amount (0 dB at drive=0, +9 dB at drive=1).
        // Peaking at 2.8 kHz, Q=2.0 → adds pick attack / upper-mid "chunk".
        const double biteDB = drive * 9.0;
        biteBoost.setCoeffs(BiquadFilter::makePeaking(2800.0, biteDB, 2.0, sr));
    }

    void rebuildTone()
    {
        if (sr <= 0.0) return;
        // High-shelf at 3 kHz: dark (-9 dB) ↔ flat ↔ bright (+9 dB)
        const double gainDB = (tone - 0.5f) * 18.0;
        toneShelf.setCoeffs(BiquadFilter::makeHighShelf(3000.0, gainDB, sr));
    }

    // ---- State --------------------------------------------------------------

    BiquadFilter tightHPF;          // Stage 1: variable HPF
    BiquadFilter biteBoost;         // Stage 2: drive-scaled peak
    BiquadFilter interHPF;          // Stage 4: fixed inter-stage HPF
    BiquadFilter toneShelf;         // Stage 7: tone brightness shelf

    // DC blocker state
    double dcR  = 0.9993;           // overwritten in prepare()
    double dcX1 = 0.0;              // x[n-1]
    double dcY1 = 0.0;              // y[n-1]

    double sr    = 44100.0;
    float  drive = 0.5f;
    float  tone  = 0.5f;
    float  tight = 0.3f;
    float  level = 0.7f;
};
