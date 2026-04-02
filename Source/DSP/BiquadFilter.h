#pragma once
#include <cmath>

// Direct Form II Transposed biquad filter.
// Coefficients computed via the RBJ Audio EQ Cookbook.
class BiquadFilter
{
public:
    struct Coeffs { double b0, b1, b2, a1, a2; };

    void setCoeffs(const Coeffs& c) noexcept
    {
        coeffs = c;
        s1 = s2 = 0.0;
    }

    void reset() noexcept { s1 = s2 = 0.0; }

    double process(double x) noexcept
    {
        double y = coeffs.b0 * x + s1;
        s1 = coeffs.b1 * x - coeffs.a1 * y + s2;
        s2 = coeffs.b2 * x - coeffs.a2 * y;
        return y;
    }

    // Low-shelf: boosts/cuts frequencies below fc.
    static Coeffs makeLowShelf(double fc, double gainDB, double sampleRate)
    {
        static constexpr double pi = 3.14159265358979323846;
        double A      = std::pow(10.0, gainDB / 40.0);
        double w0     = 2.0 * pi * fc / sampleRate;
        double cosw0  = std::cos(w0);
        double sinw0  = std::sin(w0);
        double sqrtA  = std::sqrt(A);
        double alpha  = sinw0 / 2.0 * std::sqrt((A + 1.0 / A) * (1.0 - 1.0) + 2.0);
        // shelf slope S=1 simplifies to:
        alpha = sinw0 / 2.0 * std::sqrt(2.0 * (A + 1.0 / A));

        double b0 =  A * ((A + 1) - (A - 1) * cosw0 + 2 * sqrtA * alpha);
        double b1 = 2 * A * ((A - 1) - (A + 1) * cosw0);
        double b2 =  A * ((A + 1) - (A - 1) * cosw0 - 2 * sqrtA * alpha);
        double a0 =      (A + 1) + (A - 1) * cosw0 + 2 * sqrtA * alpha;
        double a1 =   -2 * ((A - 1) + (A + 1) * cosw0);
        double a2 =      (A + 1) + (A - 1) * cosw0 - 2 * sqrtA * alpha;

        return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
    }

    // High-shelf: boosts/cuts frequencies above fc.
    static Coeffs makeHighShelf(double fc, double gainDB, double sampleRate)
    {
        static constexpr double pi = 3.14159265358979323846;
        double A      = std::pow(10.0, gainDB / 40.0);
        double w0     = 2.0 * pi * fc / sampleRate;
        double cosw0  = std::cos(w0);
        double sinw0  = std::sin(w0);
        double sqrtA  = std::sqrt(A);
        double alpha  = sinw0 / 2.0 * std::sqrt(2.0 * (A + 1.0 / A));

        double b0 =  A * ((A + 1) + (A - 1) * cosw0 + 2 * sqrtA * alpha);
        double b1 = -2 * A * ((A - 1) + (A + 1) * cosw0);
        double b2 =  A * ((A + 1) + (A - 1) * cosw0 - 2 * sqrtA * alpha);
        double a0 =      (A + 1) - (A - 1) * cosw0 + 2 * sqrtA * alpha;
        double a1 =    2 * ((A - 1) - (A + 1) * cosw0);
        double a2 =      (A + 1) - (A - 1) * cosw0 - 2 * sqrtA * alpha;

        return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
    }

    // Peaking EQ: boosts/cuts a band centered at fc with bandwidth Q.
    static Coeffs makePeaking(double fc, double gainDB, double Q, double sampleRate)
    {
        static constexpr double pi = 3.14159265358979323846;
        double A      = std::pow(10.0, gainDB / 40.0);
        double w0     = 2.0 * pi * fc / sampleRate;
        double cosw0  = std::cos(w0);
        double alpha  = std::sin(w0) / (2.0 * Q);

        double b0 =  1 + alpha * A;
        double b1 = -2 * cosw0;
        double b2 =  1 - alpha * A;
        double a0 =  1 + alpha / A;
        double a1 = -2 * cosw0;
        double a2 =  1 - alpha / A;

        return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
    }

private:
    Coeffs coeffs {1, 0, 0, 0, 0};
    double s1 = 0.0, s2 = 0.0;
};
