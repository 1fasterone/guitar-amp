#pragma once
#include "BiquadFilter.h"

// Models the "presence" control found in the power amplifier stage,
// typically implemented as a high-frequency shelving filter in the
// negative-feedback loop of the power amp.
//
// Center frequency: 4 kHz (covers the 2–8 kHz presence range)
// Control range  : 0.0 = -12 dB cut  |  0.5 = flat  |  1.0 = +12 dB boost
class PresenceFilter
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        update();
    }

    // param: 0.0 – 1.0
    void setPresence(float param) noexcept
    {
        gainDB = (param - 0.5f) * 24.0f;
        update();
    }

    double process(double x) noexcept
    {
        return filter.process(x);
    }

private:
    void update()
    {
        if (sr > 0.0)
            filter.setCoeffs(BiquadFilter::makeHighShelf(4000.0, gainDB, sr));
    }

    BiquadFilter filter;
    double sr     = 44100.0;
    double gainDB = 0.0;
};
