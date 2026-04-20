#pragma once
#include <cmath>
#include <algorithm>

// MXR Phase 90 style phaser — 4 cascaded first-order all-pass filters
// swept by a sine LFO, with a feedback path for resonance.
//
// How it works:
//   Each all-pass filter passes all frequencies at unity gain but shifts
//   their phase by 0°–180° depending on frequency. Four in series creates
//   4 notches when mixed with the dry signal. The LFO sweeps the notch
//   frequencies up and down, producing the characteristic "swoosh."
//
// Signal chain position: after Distortion/Amp, before Tape Delay.
//
// Parameters (all 0.0 – 1.0):
//   Rate      LFO speed    0 = 0.1 Hz (slow sweep)  → 1 = 8 Hz (fast)
//   Depth     LFO amount   0 = no modulation         → 1 = full sweep
//   Feedback  Resonance    0 = subtle                → 1 = intense (80% max)
//   Mix       Dry/wet      0 = dry only              → 1 = wet only
class Phaser
{
public:
    void prepare(double sampleRate)
    {
        sr          = sampleRate;
        lfoPhase    = 0.0f;

        // Reset all-pass filter states
        for (int i = 0; i < 4; ++i)
            apState[i] = 0.0f;

        fbState = 0.0f;
        updateCoeffs();
    }

    void setRate    (float v) noexcept { rate     = v; updateCoeffs(); }
    void setDepth   (float v) noexcept { depth    = v; }
    void setFeedback(float v) noexcept { feedback = v; }
    void setMix     (float v) noexcept { mix      = v; }

    float process(float input) noexcept
    {
        // --- LFO: sine wave ---
        const float lfoVal = std::sin(lfoPhase);
        lfoPhase += lfoPhaseInc;
        if (lfoPhase > 6.2831853f) lfoPhase -= 6.2831853f;

        // Map LFO (−1 to +1) to all-pass coefficient range.
        // Coefficient a controls notch frequency:
        //   a near  1.0  → notch at low  frequency (~300 Hz)
        //   a near -1.0  → notch at high frequency (~3 kHz)
        const float lfoMod = lfoVal * depth * 0.5f;
        const float a      = baseA + lfoMod;
        const float aClamp = std::max(-0.999f, std::min(0.999f, a));

        // --- Feedback: add scaled previous output back to input ---
        const float feedbackGain = feedback * 0.80f;   // hard cap at 80%
        const float x = input + fbState * feedbackGain;

        // --- 4 cascaded first-order all-pass filters ---
        // H(z) = (a + z⁻¹) / (1 + a·z⁻¹)
        // Direct Form II with one state variable per stage:
        //   w[n] = x[n] - a · w[n-1]
        //   y[n] = a · w[n] + w[n-1]   (== state before update)
        float s = x;
        for (int i = 0; i < 4; ++i)
        {
            const float w  = s - aClamp * apState[i];
            s              = aClamp * w + apState[i];
            apState[i]     = w;
        }

        fbState = s;

        // --- Dry / wet mix ---
        return input * (1.0f - mix) + s * mix;
    }

private:
    void updateCoeffs() noexcept
    {
        if (sr <= 0.0) return;

        // LFO rate: 0.1 Hz → 8 Hz (exponential taper feels natural on a knob)
        const double lfoHz = 0.1 * std::pow(80.0, (double)rate);
        static constexpr double pi = 3.14159265358979323846;
        lfoPhaseInc = (float)(2.0 * pi * lfoHz / sr);

        // Base all-pass coefficient → sets centre notch frequency.
        // a = (tan(π·fc/sr) − 1) / (tan(π·fc/sr) + 1)
        // Centre fc = 800 Hz gives the classic MXR Phase 90 character.
        const double fc   = 800.0;
        const double tanW = std::tan(pi * fc / sr);
        baseA = (float)((tanW - 1.0) / (tanW + 1.0));
    }

    double sr            = 44100.0;

    // LFO
    float lfoPhase       = 0.0f;
    float lfoPhaseInc    = 0.0f;

    // All-pass filter states (one per stage)
    float apState[4]     = {};
    float fbState        = 0.0f;

    // Pre-computed
    float baseA          = 0.0f;

    // Params
    float rate           = 0.3f;
    float depth          = 0.7f;
    float feedback       = 0.4f;
    float mix            = 0.5f;
};
