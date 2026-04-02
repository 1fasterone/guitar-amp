#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

// Tape Echo — models a vintage tape delay machine.
//
// Tape characteristics modeled:
//   • Wow/flutter   : LFO (~1.5 Hz) modulates delay time ±3 ms max
//                     gives the gentle pitch wobble of a spinning tape head
//   • HF rolloff    : one-pole low-pass in the feedback path (~4 kHz)
//                     tape oxide has limited high-frequency response
//   • Tape saturation: tanh soft-clip on the feedback signal
//                     warm, musical compression on repeat echoes
//   • Linear interpolation between samples for fractional delay times
//
// Signal chain position: after Presence, before Reverb (dry signal echoes
// into the reverb tail — classic studio routing).
//
// Parameters (all 0.0 – 1.0):
//   Time      0 = 1 ms (slapback)  →  1 = 600 ms (long echo)
//   Feedback  0 = single echo      →  1 = 85%  (near self-oscillation)
//   Mix       0 = dry only         →  1 = wet only
//   Wow       0 = perfect digital  →  1 = heavy tape wobble
class TapeDelay
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;

        // Allocate 2 s of delay headroom (covers max 600 ms + LFO excursion)
        const int maxSamples = (int)std::ceil(sampleRate * 2.0);
        buf.assign((size_t)maxSamples, 0.0f);
        writePos = 0;

        // One-pole LP coefficient for tape HF loss (~4 kHz cutoff)
        // a = exp(-2π · fc / fs)
        static constexpr double pi = 3.14159265358979323846;
        lpCoeff = (float)std::exp(-2.0 * pi * 4000.0 / sampleRate);
        lpState = 0.0f;

        // LFO phase step: 1.5 Hz wow rate
        lfoPhase    = 0.0f;
        lfoPhaseInc = (float)(2.0 * pi * 1.5 / sampleRate);

        updateCoeffs();
    }

    void setTime    (float v) noexcept { time     = v; updateCoeffs(); }
    void setFeedback(float v) noexcept { feedback = v; updateCoeffs(); }
    void setMix     (float v) noexcept { mix      = v; }
    void setWow     (float v) noexcept { wow      = v; updateCoeffs(); }

    // Mono in / mono out — insert before the stereo reverb.
    float process(float input) noexcept
    {
        if (buf.empty()) return input;

        // --- Wow/flutter: LFO modulates delay time ---
        const float lfoSin = std::sin(lfoPhase);
        lfoPhase += lfoPhaseInc;
        if (lfoPhase > 6.2831853f) lfoPhase -= 6.2831853f;

        // delaySamples varies ±wowDepthSamples around baseDelaySamples
        const float delaySamples = baseDelaySamples + lfoSin * wowDepthSamples;

        // --- Read with linear interpolation ---
        const float  frac  = delaySamples - std::floor(delaySamples);
        const int    iDelay = (int)delaySamples;
        const int    bufLen = (int)buf.size();

        int r0 = writePos - iDelay;
        if (r0 < 0) r0 += bufLen;
        int r1 = r0 - 1;
        if (r1 < 0) r1 += bufLen;

        const float delayed = buf[(size_t)r0] * (1.0f - frac)
                            + buf[(size_t)r1] * frac;

        // --- Tape feedback path ---
        // 1. High-frequency rolloff (tape oxide HF loss)
        lpState = (1.0f - lpCoeff) * delayed + lpCoeff * lpState;

        // 2. Tape saturation — soft-clip with tanh, then re-normalise
        //    Boosting ×1.5 before tanh adds gentle harmonic warmth;
        //    dividing restores approximate unity through the saturator.
        const float saturated = std::tanh(lpState * 1.5f) * 0.667f;

        // 3. Apply feedback gain (capped at 0.85 — prevents runaway)
        const float feedbackSig = saturated * feedbackGain;

        // --- Write new sample into circular buffer ---
        buf[(size_t)writePos] = input + feedbackSig;
        if (++writePos >= bufLen) writePos = 0;

        // --- Dry / wet mix ---
        return input * dry + delayed * wet;
    }

private:
    void updateCoeffs() noexcept
    {
        // Time: 1 ms → 600 ms, exponential taper for musical spacing
        // linear 0-1 maps to log range so short times feel responsive
        const double minMs = 1.0,  maxMs = 600.0;
        const double ms = minMs * std::pow(maxMs / minMs, (double)time);
        baseDelaySamples = (float)(ms * 0.001 * sr);

        // Wow depth: 0 = 0 ms, 1 = ±3 ms wobble
        wowDepthSamples = wow * (float)(0.003 * sr);

        // Hard cap feedback at 85 % to prevent self-oscillation
        feedbackGain = std::min(feedback, 0.85f);

        wet = mix;
        dry = 1.0f - mix;
    }

    std::vector<float> buf;
    int   writePos          = 0;

    // LFO
    float lfoPhase          = 0.0f;
    float lfoPhaseInc       = 0.0f;

    // One-pole LP (tape HF loss)
    float lpCoeff           = 0.0f;
    float lpState           = 0.0f;

    double sr               = 44100.0;

    // Params
    float time              = 0.35f;
    float feedback          = 0.40f;
    float mix               = 0.30f;
    float wow               = 0.25f;

    // Pre-computed
    float baseDelaySamples  = 0.0f;
    float wowDepthSamples   = 0.0f;
    float feedbackGain      = 0.40f;
    float wet               = 0.30f;
    float dry               = 0.70f;
};
