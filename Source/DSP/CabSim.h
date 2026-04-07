#pragma once
#include <JuceHeader.h>

// =============================================================================
// CabSim — zero-latency IR convolution cabinet simulator
//
// Audio thread calls processBlock() each callback.
// Message thread calls loadIR() when the user picks a file.
// juce::dsp::Convolution handles cross-thread safety internally via a
// lock-free command queue; the IR FFT is pre-computed on a background thread.
// =============================================================================
class CabSim
{
public:
    void prepare(double sampleRate, int maxBlockSize)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate       = sampleRate;
        spec.maximumBlockSize = (juce::uint32)maxBlockSize;
        spec.numChannels      = 1;          // mono in → mono out
        convolution.prepare(spec);
        prepared = true;
    }

    // Call from the message thread after the user selects a file.
    void loadIR(const juce::File& file)
    {
        convolution.loadImpulseResponse(
            file,
            juce::dsp::Convolution::Stereo::no,      // fold to mono
            juce::dsp::Convolution::Trim::yes,        // strip leading silence
            0,                                        // 0 = full IR length
            juce::dsp::Convolution::Normalise::yes);  // peak-normalise IR
        irLoaded = true;
    }

    // Process a mono block in-place.
    //   blend = 0.0 → fully dry (bypass)
    //   blend = 1.0 → fully processed through the cabinet IR
    void processBlock(float* data, int numSamples, float blend) noexcept
    {
        if (!prepared || !irLoaded || blend <= 0.0f)
            return;

        // Keep dry copy for blend
        if (blend < 1.0f)
        {
            dryBuf.resize((size_t)numSamples);
            juce::FloatVectorOperations::copy(dryBuf.data(), data, numSamples);
        }

        // Convolution (in-place)
        float* ch[] = { data };
        juce::dsp::AudioBlock<float>              block(ch, 1, (size_t)numSamples);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        convolution.process(ctx);

        // Wet/dry blend
        if (blend < 1.0f)
        {
            const float wet = blend;
            const float dry = 1.0f - blend;
            juce::FloatVectorOperations::multiply(data, wet, numSamples);
            juce::FloatVectorOperations::addWithMultiply(
                data, dryBuf.data(), dry, numSamples);
        }
    }

    bool isLoaded() const noexcept { return irLoaded; }

private:
    juce::dsp::Convolution convolution;   // default ctor = zero added latency
    std::vector<float>     dryBuf;
    bool prepared = false;
    bool irLoaded = false;
};
