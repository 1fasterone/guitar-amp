#pragma once

// Post-chain output level control. A simple linear gain scalar applied
// after all processing stages, preserving tonal character while
// controlling overall volume.
//
// A square-law mapping (gain = v²) is used so the knob feels natural:
// the bottom half of rotation covers the quiet range, the top half
// covers normal playing levels.
//
// Control: 0.0 = silence  |  0.5 = ~25% power  |  1.0 = unity gain (0 dB)
class MasterVolume
{
public:
    // param: 0.0 – 1.0
    void setMaster(float param) noexcept
    {
        gain = param * param;   // square law
    }

    float process(float x) noexcept
    {
        return x * gain;
    }

private:
    float gain = 1.0f;
};
