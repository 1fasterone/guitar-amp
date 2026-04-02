#pragma once
#include <cmath>

// Soft-clipping preamp stage using a tanh waveshaper.
//
// drive range [1, 20]:
//   1   = unity gain, no harmonic distortion
//   ~5  = mild overdrive
//   20  = heavy saturation / fuzz
//
// The output is normalized so that a sine wave at full scale doesn't
// change in peak level — only the harmonic content changes.
class GainStage
{
public:
    // param: 0.0 (clean) to 1.0 (full drive)
    void setGain(float param) noexcept
    {
        drive = 1.0f + param * 19.0f;
        // Pre-compute normalization so hot signals stay in range.
        // tanh(drive) is the max output for a +1.0 input.
        norm = 1.0f / std::tanh(drive);
    }

    float process(float x) noexcept
    {
        return std::tanh(drive * x) * norm;
    }

private:
    float drive = 1.0f;
    float norm  = 1.0f;
};
