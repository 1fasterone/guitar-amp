#pragma once
#include <cmath>

// Noise Gate — silences the signal when it drops below a threshold.
//
// Placed FIRST in the chain (before distortion) so hum, hiss, and pick
// noise are removed before the gain stage amplifies them.
//
// State machine:
//   CLOSED  → signal muted; if RMS > threshold → ATTACK
//   ATTACK  → gain ramps to 1.0 over attack time; when open → HOLD
//   HOLD    → gate stays open for hold duration (prevents chatter
//              on fast picking); if RMS still > threshold, hold resets
//   RELEASE → gain ramps to 0.0 over release time → CLOSED
//
// Parameters (all 0.0 – 1.0):
//   Threshold   signal level that opens/closes the gate
//               0 = -80 dBFS (almost always open)
//               1 = -20 dBFS (aggressive, cuts everything quiet)
//   Attack      how fast the gate opens   0 = 0.1 ms  → 1 = 50 ms
//   Release     how fast the gate closes  0 = 10 ms   → 1 = 500 ms
//   Hold        how long gate stays open after signal drops
//               0 = 0 ms  → 1 = 500 ms  (prevents chatter on picking)
class NoiseGate
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        // RMS detector: ~5 ms smoothing window
        rmsAlpha = (float)std::exp(-1.0 / (0.005 * sampleRate));
        rmsLevel = 0.0f;
        gain     = 0.0f;
        state    = State::Closed;
        holdCountdown = 0.0f;
        updateCoeffs();
    }

    void setThreshold(float v) noexcept { threshold = v; updateCoeffs(); }
    void setAttack   (float v) noexcept { attack    = v; updateCoeffs(); }
    void setRelease  (float v) noexcept { release   = v; updateCoeffs(); }
    void setHold     (float v) noexcept { hold      = v; updateCoeffs(); }

    float process(float input) noexcept
    {
        // --- RMS level detector ---
        rmsLevel = std::sqrt(rmsAlpha * rmsLevel * rmsLevel
                           + (1.0f - rmsAlpha) * input * input);

        // --- State machine ---
        switch (state)
        {
            case State::Closed:
                gain = 0.0f;
                if (rmsLevel > thresholdLinear)
                {
                    state = State::Attack;
                }
                break;

            case State::Attack:
                gain += attackCoeff * (1.0f - gain);
                if (gain >= 0.999f)
                {
                    gain = 1.0f;
                    holdCountdown = holdSamples;
                    state = State::Hold;
                }
                break;

            case State::Hold:
                gain = 1.0f;
                if (rmsLevel > thresholdLinear)
                    holdCountdown = holdSamples;   // signal still loud: reset hold
                else if (--holdCountdown <= 0.0f)
                    state = State::Release;
                break;

            case State::Release:
                gain += releaseCoeff * (0.0f - gain);
                if (rmsLevel > thresholdLinear)
                {
                    state = State::Attack;   // signal came back — reopen
                }
                else if (gain <= 0.001f)
                {
                    gain  = 0.0f;
                    state = State::Closed;
                }
                break;
        }

        return input * gain;
    }

    // For UI display: current gain reduction in dB (0 = open, negative = gating)
    float getGainReductionDB() const noexcept
    {
        if (gain <= 0.0001f) return -80.0f;
        return 20.0f * std::log10(gain);
    }

private:
    void updateCoeffs() noexcept
    {
        if (sr <= 0.0) return;

        // Threshold: 0 → -80 dBFS,  1 → -20 dBFS
        const float threshDB = -80.0f + threshold * 60.0f;
        thresholdLinear = std::pow(10.0f, threshDB / 20.0f);

        // Attack:  0.1 ms → 50 ms  (exponential)
        const double attackMs  = 0.1  * std::pow(500.0,  (double)attack);
        // Release: 10 ms  → 500 ms (exponential)
        const double releaseMs = 10.0 * std::pow(50.0,   (double)release);

        // One-pole coefficients: coeff = 1 - exp(-1 / (time_s * sr))
        attackCoeff  = (float)(1.0 - std::exp(-1.0 / (attackMs  * 0.001 * sr)));
        releaseCoeff = (float)(1.0 - std::exp(-1.0 / (releaseMs * 0.001 * sr)));

        // Hold: 0 ms → 500 ms (linear)
        holdSamples = hold * (float)(0.5 * sr);
    }

    enum class State { Closed, Attack, Hold, Release };

    double sr             = 44100.0;

    // Level detector
    float  rmsAlpha       = 0.0f;
    float  rmsLevel       = 0.0f;

    // Gate envelope
    State  state          = State::Closed;
    float  gain           = 0.0f;
    float  holdCountdown  = 0.0f;

    // Pre-computed
    float  thresholdLinear = 0.0f;
    float  attackCoeff     = 0.0f;
    float  releaseCoeff    = 0.0f;
    float  holdSamples     = 0.0f;

    // Params
    float  threshold = 0.3f;
    float  attack    = 0.2f;
    float  release   = 0.5f;
    float  hold      = 0.3f;
};
