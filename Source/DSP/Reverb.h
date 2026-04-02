#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

// Freeverb-style stereo algorithmic reverb.
//
// Architecture (Schroeder / Moorer):
//   Mono input
//     │
//   8 parallel feedback comb filters (left)  ─┐
//   8 parallel feedback comb filters (right) ─┤  run simultaneously
//     │                                        │
//   Sum left combs ──► 4 serial allpass (L)   │
//   Sum right combs ─► 4 serial allpass (R)   │
//     │                                        │
//   Stereo width matrix ─────────────────────-─┘
//     │
//   Dry/wet mix
//     │
//   Stereo output (outL, outR)
//
// Parameters (all 0.0 – 1.0):
//   Mix   — dry/wet balance  (0 = dry only,  1 = wet only)
//   Size  — room size / tail length
//   Damp  — high-frequency damping in feedback path (0 = bright, 1 = dark)
//   Width — stereo spread    (0 = mono wet,  1 = full stereo)
class FreeverbEngine
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;

        // Delay-line lengths (samples) at 44100 Hz — canonical Freeverb tuning.
        // Right channel = left + spread (23 samples, scaled to sample rate).
        static constexpr int combTuningL[8]    = { 1116, 1188, 1277, 1356,
                                                    1422, 1491, 1557, 1617 };
        static constexpr int allpassTuningL[4] = { 556, 441, 341, 225 };
        static constexpr int kSpreadAt44k      = 23;

        const int spread = (int)std::round(kSpreadAt44k * sr / 44100.0);

        for (int i = 0; i < 8; ++i)
        {
            int lenL = (int)std::round(combTuningL[i] * sr / 44100.0);
            combL[i].resize(lenL);
            combR[i].resize(lenL + spread);
        }
        for (int i = 0; i < 4; ++i)
        {
            int lenL = (int)std::round(allpassTuningL[i] * sr / 44100.0);
            allpassL[i].resize(lenL);
            allpassR[i].resize(lenL + spread);
        }

        updateCoeffs();
    }

    void setMix  (float v) noexcept { mix   = v; updateCoeffs(); }
    void setSize (float v) noexcept { size  = v; updateCoeffs(); }
    void setDamp (float v) noexcept { damp  = v; updateCoeffs(); }
    void setWidth(float v) noexcept { width = v; updateCoeffs(); }

    // Stereo process: takes mono input, writes independent L and R samples.
    void process(float monoIn, float& outL, float& outR) noexcept
    {
        // Spread input energy across comb filters equally.
        // The * 0.015f pre-scale avoids internal saturation; the wet gain
        // compensates by multiplying back up (see updateCoeffs, wet = mix * 3).
        const float input = monoIn * 0.015f;

        // 1. Eight parallel comb filters
        float rawL = 0.0f, rawR = 0.0f;
        for (int i = 0; i < 8; ++i)
        {
            rawL += combL[i].process(input);
            rawR += combR[i].process(input);
        }

        // 2. Four serial allpass diffusers
        for (int i = 0; i < 4; ++i)
        {
            rawL = allpassL[i].process(rawL);
            rawR = allpassR[i].process(rawR);
        }

        // 3. Stereo width matrix
        //    wetScaleL = (1+width)*0.5 → 0.5 (mono) to 1.0 (full stereo)
        //    wetScaleR = (1-width)*0.5 → 0.5 (mono) to 0.0 (full stereo)
        const float wetL = rawL * wetScaleL + rawR * wetScaleR;
        const float wetR = rawR * wetScaleL + rawL * wetScaleR;

        // 4. Dry / wet mix
        outL = monoIn * dry + wetL * wet;
        outR = monoIn * dry + wetR * wet;
    }

private:
    //==========================================================================
    // Feedback comb filter with one-pole low-pass in the feedback path.
    // The LP models air absorption at high frequencies (damping).
    struct CombFilter
    {
        std::vector<float> buf;
        int   readPos    = 0;
        float filterStore = 0.0f;
        float feedback   = 0.5f;
        float damp1      = 0.2f;   // smoothing weight (LP feedback coeff)
        float damp2      = 0.8f;   // passthrough weight

        void resize(int len)
        {
            buf.assign((size_t)len, 0.0f);
            readPos    = 0;
            filterStore = 0.0f;
        }

        void set(float fb, float d1)
        {
            feedback = fb;
            damp1    = d1;
            damp2    = 1.0f - d1;
        }

        float process(float input) noexcept
        {
            const float output = buf[(size_t)readPos];
            filterStore = output * damp2 + filterStore * damp1;
            buf[(size_t)readPos] = input + filterStore * feedback;
            if (++readPos >= (int)buf.size()) readPos = 0;
            return output;
        }
    };

    //==========================================================================
    // Schroeder allpass filter — flat magnitude response, diffuses echo density.
    // g = 0.5 is the canonical Freeverb coefficient for all 4 stages.
    struct AllpassFilter
    {
        std::vector<float> buf;
        int readPos = 0;

        void resize(int len)
        {
            buf.assign((size_t)len, 0.0f);
            readPos = 0;
        }

        float process(float input) noexcept
        {
            const float bufOut = buf[(size_t)readPos];
            const float output = -input + bufOut;
            buf[(size_t)readPos] = input + bufOut * 0.5f;
            if (++readPos >= (int)buf.size()) readPos = 0;
            return output;
        }
    };

    //==========================================================================
    void updateCoeffs() noexcept
    {
        // Comb filter parameters
        // feedback : 0.5 (small room) → 0.78 (large room / long tail)
        // damp1    : 0.0 (bright)     → 0.4  (dark / air-absorbed)
        const float feedback = 0.5f + size * 0.28f;
        const float damp1    = damp  * 0.40f;

        for (int i = 0; i < 8; ++i)
        {
            combL[i].set(feedback, damp1);
            combR[i].set(feedback, damp1);
        }

        // Width matrix coefficients
        wetScaleL = (1.0f + width) * 0.5f;
        wetScaleR = (1.0f - width) * 0.5f;

        // Wet gain × 3.0: compensates for the 0.015 pre-scale applied in process()
        // and normalises the sum of 8 comb filters to approximately unity.
        wet = mix * 3.0f;
        dry = 1.0f - mix;
    }

    //==========================================================================
    CombFilter   combL[8],    combR[8];
    AllpassFilter allpassL[4], allpassR[4];

    double sr       = 44100.0;
    float  mix      = 0.25f;
    float  size     = 0.50f;
    float  damp     = 0.50f;
    float  width    = 1.00f;

    // Pre-computed from updateCoeffs()
    float wet       = 0.75f;
    float dry       = 0.75f;
    float wetScaleL = 1.00f;
    float wetScaleR = 0.00f;
};
