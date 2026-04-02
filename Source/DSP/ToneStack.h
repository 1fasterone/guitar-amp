#pragma once
#include "BiquadFilter.h"

// Three-band tone stack modeled as independent biquad filters.
//
// Bands:
//   Bass   — low shelf  at  120 Hz
//   Mid    — peaking EQ at  800 Hz (Q = 1.5)
//   Treble — high shelf at 3500 Hz
//
// Each control: 0.0 = -12 dB cut, 0.5 = flat, 1.0 = +12 dB boost
class ToneStack
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        updateAll();
    }

    void setBass  (float v) noexcept { bassGain   = (v - 0.5f) * 24.0f; updateBass();   }
    void setMid   (float v) noexcept { midGain    = (v - 0.5f) * 24.0f; updateMid();    }
    void setTreble(float v) noexcept { trebleGain = (v - 0.5f) * 24.0f; updateTreble(); }

    // Call from the audio thread only.
    double process(double x) noexcept
    {
        return treble.process(mid.process(bass.process(x)));
    }

private:
    void updateBass()   { if (sr > 0) bass.setCoeffs(  BiquadFilter::makeLowShelf (120.0,  bassGain,   sr)); }
    void updateMid()    { if (sr > 0) mid.setCoeffs(   BiquadFilter::makePeaking  (800.0,  midGain, 1.5, sr)); }
    void updateTreble() { if (sr > 0) treble.setCoeffs(BiquadFilter::makeHighShelf(3500.0, trebleGain, sr)); }
    void updateAll()    { updateBass(); updateMid(); updateTreble(); }

    BiquadFilter bass, mid, treble;
    double sr         = 44100.0;
    double bassGain   = 0.0;
    double midGain    = 0.0;
    double trebleGain = 0.0;
};
