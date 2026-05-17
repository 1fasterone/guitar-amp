#pragma once
#include <JuceHeader.h>
#include <cmath>

/**  Auto-wah using a Chamberlin state-variable filter (bandpass output).
 *
 *   An envelope follower tracks the input amplitude and sweeps the filter's
 *   centre frequency upward proportionally — the classic "auto-wah" envelope
 *   response.  Works in mono; call once per sample in your processing loop.
 *
 *   All four parameters accept a normalised [0, 1] value:
 *     freq  — centre/base frequency  (0 → 200 Hz, 1 → 3 000 Hz, log spread)
 *     q     — resonance sharpness    (0 → Q≈1 gentle,  1 → Q≈8 very sharp)
 *     sens  — envelope sensitivity   (0 = static wah,  1 = full auto-wah)
 *     mix   — wet / dry              (0 = bypass,       1 = fully wet)
 */
class WahWah
{
public:
    void prepare (double sampleRate)
    {
        sr = (float)sampleRate;
        reset();
    }

    void reset()
    {
        envFol = 0.0f;
        s1     = 0.0f;   // SVF band state
        s2     = 0.0f;   // SVF low state
    }

    float process (float x,
                   float freq,   // [0, 1]
                   float q,      // [0, 1]
                   float sens,   // [0, 1]
                   float mix)    // [0, 1]
    {
        // ---- envelope follower -----------------------------------------------
        const float absX = std::abs (x);
        //  fast attack (~1 ms @ 44.1 kHz), slow release (~250 ms)
        const float att = 0.005f;
        const float rel = 0.0008f;
        envFol += (absX > envFol) ? att * (absX - envFol)
                                  : rel * (absX - envFol);

        // ---- parameter mapping -----------------------------------------------
        //  frequency: 200 Hz … 3 000 Hz on a logarithmic curve
        const float freqHz    = 200.0f * std::pow (15.0f, freq);
        //  resonance Q: 1 … 8
        const float qR        = 1.0f + q * 7.0f;
        //  envelope pushes frequency upward (max ×5 at full sensitivity)
        const float modFreqHz = juce::jlimit (80.0f, 4800.0f,
                                    freqHz * (1.0f + sens * envFol * 5.0f));

        // ---- Chamberlin state-variable filter (bandpass tap) -----------------
        //  f coefficient — clamped for stability
        const float f  = juce::jmin (
                             2.0f * std::sin (juce::MathConstants<float>::pi
                                               * modFreqHz / sr),
                             1.80f);
        const float kd = 1.0f / qR;   // damping (inverse of resonance Q)

        const float high = x  - kd * s1 - s2;
        const float band = f  * high + s1;
        const float low  = f  * band + s2;
        s1 = band;
        s2 = low;

        // ---- wet / dry mix --------------------------------------------------
        return mix * band + (1.0f - mix) * x;
    }

private:
    float sr     { 44100.0f };
    float envFol { 0.0f };
    float s1     { 0.0f };
    float s2     { 0.0f };
};
