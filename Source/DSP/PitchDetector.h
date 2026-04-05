#pragma once
#include <cmath>
#include <array>
#include <atomic>
#include <algorithm>

// Chromatic pitch detector using Normalised Square Difference Function (NSDF).
//
// Audio thread calls write() every sample.
// UI thread calls analyze() in a timer callback (~15 fps).
// Results are communicated via std::atomic — no locks needed.
//
// Detection range: 60 Hz – 1300 Hz  (covers all 6 guitar strings, all frets)
// Buffer size    : 4096 samples  (~85 ms at 48 kHz — enough for low E string)
class PitchDetector
{
public:
    static constexpr int kBufSize = 4096;

    void setSampleRate(double sampleRate) noexcept { sr = sampleRate; }

    // ---- Audio thread -------------------------------------------------------
    void write(float sample) noexcept
    {
        buf[writePos] = sample;
        writePos = (writePos + 1) & (kBufSize - 1);   // power-of-2 wrap
        if (writePos == 0)
            newData.store(true, std::memory_order_release);
    }

    // ---- UI / timer thread --------------------------------------------------
    // Returns true when a new pitch estimate is ready.
    bool analyze(float& outFreqHz, int& outMidiNote, float& outCents) noexcept
    {
        if (!newData.exchange(false, std::memory_order_acquire))
            return false;

        // Snapshot (mild race acceptable — tuner doesn't need sample accuracy)
        std::array<float, kBufSize> local = buf;

        float freq = runNSDF(local);
        if (freq <= 0.0f)
            return false;

        outFreqHz  = freq;
        float midi = 12.0f * std::log2(freq / 440.0f) + 69.0f;
        outMidiNote = (int)std::round(midi);
        outCents    = (midi - (float)outMidiNote) * 100.0f;
        return true;
    }

    // ---- Helpers ------------------------------------------------------------
    static const char* noteName(int midiNote) noexcept
    {
        static const char* n[] = { "C","C#","D","D#","E","F",
                                   "F#","G","G#","A","A#","B" };
        if (midiNote < 0 || midiNote > 127) return "-";
        return n[midiNote % 12];
    }

    static int noteOctave(int midiNote) noexcept
    {
        return midiNote / 12 - 1;
    }

private:
    float runNSDF(const std::array<float, kBufSize>& s) noexcept
    {
        const int N = kBufSize;

        // Silence check
        float rms = 0.0f;
        for (float x : s) rms += x * x;
        if (rms / N < 0.0001f) return 0.0f;   // ~-40 dBFS threshold

        int minLag = (int)(sr / 1300.0 + 0.5);
        int maxLag = std::min((int)(sr / 60.0 + 0.5), N / 2 - 1);

        // NSDF: 2·r(τ) / (m²(0) + m²(τ))  where m²(τ) = Σ[x² + x_τ²]
        float best   = 0.0f;
        int   bestLag = -1;

        for (int lag = minLag; lag <= maxLag; ++lag)
        {
            double corr = 0.0, norm = 0.0;
            for (int i = 0; i < N - lag; ++i)
            {
                corr += (double)s[i] * s[i + lag];
                norm += (double)s[i] * s[i] + (double)s[i + lag] * s[i + lag];
            }
            float nsdf = (norm > 1e-10) ? (float)(2.0 * corr / norm) : 0.0f;

            if (nsdf > best) { best = nsdf; bestLag = lag; }
        }

        if (best < 0.25f || bestLag < minLag) return 0.0f;

        // Parabolic interpolation for sub-sample frequency accuracy
        float fracLag = (float)bestLag;
        if (bestLag > minLag && bestLag < maxLag)
        {
            auto nsdfAt = [&](int lag) -> float
            {
                double c = 0.0, m = 0.0;
                for (int i = 0; i < N - lag; ++i)
                {
                    c += (double)s[i] * s[i + lag];
                    m += (double)s[i] * s[i] + (double)s[i + lag] * s[i + lag];
                }
                return (m > 1e-10) ? (float)(2.0 * c / m) : 0.0f;
            };
            float y1 = nsdfAt(bestLag - 1);
            float y2 = best;
            float y3 = nsdfAt(bestLag + 1);
            float denom = 2.0f * (2.0f * y2 - y1 - y3);
            if (std::abs(denom) > 1e-6f)
                fracLag = (float)bestLag + (y3 - y1) / denom;
        }

        return (float)(sr / fracLag);
    }

    std::array<float, kBufSize> buf {};
    int  writePos = 0;
    std::atomic<bool> newData { false };
    double sr = 44100.0;
};
