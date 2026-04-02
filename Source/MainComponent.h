#pragma once
#include <JuceHeader.h>
#include "DSP/PowerMetalDistortion.h"
#include "DSP/GainStage.h"
#include "DSP/ToneStack.h"
#include "DSP/PresenceFilter.h"
#include "DSP/TapeDelay.h"
#include "DSP/Reverb.h"
#include "DSP/MasterVolume.h"

// ============================================================================
// Full signal chain:
//
//   Live Input (mono)
//      │
//   [PowerMetalDistortion]   Drive / Tone / Tight / Level
//      │
//   [GainStage]              tanh waveshaper          (Gain)
//      │
//   [ToneStack]              Bass / Mid / Treble
//      │
//   [PresenceFilter]         4 kHz high-shelf          (Presence)
//      │
//   [TapeDelay]              Wow/flutter tape echo     (Time/Feedback/Mix/Wow)
//      │
//   [FreeverbEngine]         Freeverb stereo           (Mix/Size/Damp/Width)
//      │
//   [MasterVolume]           linear scalar             (Master)
//      │
//   Stereo Output (L + R)
// ============================================================================
class MainComponent : public juce::AudioAppComponent
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void setupKnob(juce::Slider& knob, juce::Label& label,
                   const juce::String& name, double defaultVal,
                   juce::Colour accentColour);

    //==========================================================================
    // UI — distortion row
    juce::Slider driveKnob, toneKnob, tightKnob, distLevelKnob;
    juce::Label  driveLabel, toneLabel, tightLabel, distLevelLabel;
    juce::Label  distSectionLabel;

    // UI — tape delay row
    juce::Slider delayTimeKnob, delayFeedbackKnob, delayMixKnob, delayWowKnob;
    juce::Label  delayTimeLabel, delayFeedbackLabel, delayMixLabel, delayWowLabel;
    juce::Label  delaySectionLabel;

    // UI — amp preamp
    juce::Slider gainKnob, bassKnob, midKnob, trebleKnob;
    juce::Label  gainLabel, bassLabel, midLabel, trebleLabel;
    juce::Label  preampSectionLabel;

    // UI — amp power
    juce::Slider presenceKnob, masterKnob;
    juce::Label  presenceLabel, masterLabel;
    juce::Label  powerAmpSectionLabel;

    // UI — reverb
    juce::Slider reverbMixKnob, reverbSizeKnob, reverbDampKnob, reverbWidthKnob;
    juce::Label  reverbMixLabel, reverbSizeLabel, reverbDampLabel, reverbWidthLabel;
    juce::Label  reverbSectionLabel;

    // UI — audio settings button
    juce::TextButton audioSettingsButton { "AUDIO SETTINGS" };

    //==========================================================================
    // DSP — audio thread only
    PowerMetalDistortion powerMetalDist;
    GainStage            gainStage;
    ToneStack            toneStack;
    PresenceFilter       presenceFilter;
    TapeDelay            tapeDelay;
    FreeverbEngine       reverb;
    MasterVolume         masterVolume;

    //==========================================================================
    // Atomic params: UI thread writes → audio thread reads once per block
    std::atomic<float> pDrive          {0.5f};
    std::atomic<float> pTone           {0.5f};
    std::atomic<float> pTight          {0.3f};
    std::atomic<float> pDistLevel      {0.7f};

    std::atomic<float> pDelayTime      {0.35f};
    std::atomic<float> pDelayFeedback  {0.40f};
    std::atomic<float> pDelayMix       {0.30f};
    std::atomic<float> pDelayWow       {0.25f};

    std::atomic<float> pGain           {0.5f};
    std::atomic<float> pBass           {0.5f};
    std::atomic<float> pMid            {0.5f};
    std::atomic<float> pTreble         {0.5f};
    std::atomic<float> pPresence       {0.5f};
    std::atomic<float> pMaster         {0.8f};

    std::atomic<float> pReverbMix      {0.25f};
    std::atomic<float> pReverbSize     {0.50f};
    std::atomic<float> pReverbDamp     {0.50f};
    std::atomic<float> pReverbWidth    {1.00f};

    std::atomic<bool>  paramsChanged   {false};
    std::atomic<bool>  dspReady        {false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
