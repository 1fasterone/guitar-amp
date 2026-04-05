#pragma once
#include <JuceHeader.h>
#include "DSP/NoiseGate.h"
#include "DSP/PowerMetalDistortion.h"
#include "DSP/GainStage.h"
#include "DSP/ToneStack.h"
#include "DSP/PresenceFilter.h"
#include "DSP/Phaser.h"
#include "DSP/TapeDelay.h"
#include "DSP/Reverb.h"
#include "DSP/MasterVolume.h"
#include "DSP/PitchDetector.h"

// ============================================================================
// Signal chain:
//   Input → [NoiseGate] → [Distortion] → [Gain] → [ToneStack] → [Presence]
//         → [Phaser] → [TapeDelay] → [Reverb] → [Master] → Stereo Out
//   + Backing track mixed into stereo output
// ============================================================================
class MainComponent : public juce::AudioAppComponent,
                      private juce::Timer
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
    void timerCallback() override;

    void setupKnob(juce::Slider& knob, juce::Label& nameLabel,
                   juce::Label& valueLabel,
                   const juce::String& name, double defaultVal,
                   juce::Colour accentColour);

    juce::String formatValue(const juce::String& name, double v);

    //==========================================================================
    // UI — noise gate
    juce::Slider gateThreshKnob, gateAttackKnob, gateReleaseKnob, gateHoldKnob;
    juce::Label  gateThreshLabel,    gateAttackLabel,    gateReleaseLabel,    gateHoldLabel;
    juce::Label  gateThreshValLabel, gateAttackValLabel, gateReleaseValLabel, gateHoldValLabel;
    juce::Label  gateSectionLabel;

    // UI — distortion
    juce::Slider driveKnob, toneKnob, tightKnob, distLevelKnob;
    juce::Label  driveLabel,    toneLabel,    tightLabel,    distLevelLabel;
    juce::Label  driveValLabel, toneValLabel, tightValLabel, distLevelValLabel;
    juce::Label  distSectionLabel;

    // UI — phaser
    juce::Slider phaserRateKnob, phaserDepthKnob, phaserFeedbackKnob, phaserMixKnob;
    juce::Label  phaserRateLabel,     phaserDepthLabel,     phaserFeedbackLabel,     phaserMixLabel;
    juce::Label  phaserRateValLabel,  phaserDepthValLabel,  phaserFeedbackValLabel,  phaserMixValLabel;
    juce::Label  phaserSectionLabel;

    // UI — tape delay
    juce::Slider delayTimeKnob, delayFeedbackKnob, delayMixKnob, delayWowKnob;
    juce::Label  delayTimeLabel,     delayFeedbackLabel,     delayMixLabel,     delayWowLabel;
    juce::Label  delayTimeValLabel,  delayFeedbackValLabel,  delayMixValLabel,  delayWowValLabel;
    juce::Label  delaySectionLabel;

    // UI — amp preamp
    juce::Slider gainKnob, bassKnob, midKnob, trebleKnob;
    juce::Label  gainLabel,    bassLabel,    midLabel,    trebleLabel;
    juce::Label  gainValLabel, bassValLabel, midValLabel, trebleValLabel;
    juce::Label  preampSectionLabel;

    // UI — amp power
    juce::Slider presenceKnob, masterKnob;
    juce::Label  presenceLabel,    masterLabel;
    juce::Label  presenceValLabel, masterValLabel;
    juce::Label  powerAmpSectionLabel;

    // UI — reverb
    juce::Slider reverbMixKnob, reverbSizeKnob, reverbDampKnob, reverbWidthKnob;
    juce::Label  reverbMixLabel,     reverbSizeLabel,     reverbDampLabel,     reverbWidthLabel;
    juce::Label  reverbMixValLabel,  reverbSizeValLabel,  reverbDampValLabel,  reverbWidthValLabel;
    juce::Label  reverbSectionLabel;

    // UI — chromatic tuner
    juce::TextButton tunerOnButton  { "TUNER: OFF" };
    juce::Label      tunerNoteLabel;      // shows "E4", "A3", etc. (big font)
    juce::Label      tunerCentsLabel;     // shows "+2 cents"
    juce::Label      tunerFreqLabel;      // shows "329.6 Hz"
    juce::Label      tunerSectionLabel;

    // UI — backing track player
    juce::TextButton btLoadButton { "LOAD" };
    juce::TextButton btPlayButton { "PLAY" };
    juce::TextButton btLoopButton { "LOOP: OFF" };
    juce::Slider     btVolumeKnob;
    juce::Label      btVolumeLabel, btVolumeValLabel;
    juce::Label      btFileLabel;
    juce::Label      btSectionLabel;

    // Audio settings button
    juce::TextButton audioSettingsButton { ">>  AUDIO SETTINGS" };

    //==========================================================================
    // DSP — audio thread only
    NoiseGate            noiseGate;
    PowerMetalDistortion powerMetalDist;
    GainStage            gainStage;
    ToneStack            toneStack;
    PresenceFilter       presenceFilter;
    Phaser               phaser;
    TapeDelay            tapeDelay;
    FreeverbEngine       reverb;
    MasterVolume         masterVolume;
    PitchDetector        pitchDetector;

    // Backing track audio engine
    juce::AudioFormatManager                       formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource                     transportSource;
    std::unique_ptr<juce::FileChooser>             fileChooser;
    juce::AudioBuffer<float>                       backingBuffer;

    //==========================================================================
    // Atomic params — guitar DSP
    std::atomic<float> pGateThresh    {0.30f};
    std::atomic<float> pGateAttack    {0.20f};
    std::atomic<float> pGateRelease   {0.50f};
    std::atomic<float> pGateHold      {0.30f};

    std::atomic<float> pDrive         {0.50f};
    std::atomic<float> pTone          {0.50f};
    std::atomic<float> pTight         {0.30f};
    std::atomic<float> pDistLevel     {0.70f};

    std::atomic<float> pPhaserRate    {0.30f};
    std::atomic<float> pPhaserDepth   {0.70f};
    std::atomic<float> pPhaserFeedback{0.40f};
    std::atomic<float> pPhaserMix     {0.50f};

    std::atomic<float> pDelayTime     {0.35f};
    std::atomic<float> pDelayFeedback {0.40f};
    std::atomic<float> pDelayMix      {0.30f};
    std::atomic<float> pDelayWow      {0.25f};

    std::atomic<float> pGain          {0.50f};
    std::atomic<float> pBass          {0.50f};
    std::atomic<float> pMid           {0.50f};
    std::atomic<float> pTreble        {0.50f};
    std::atomic<float> pPresence      {0.50f};
    std::atomic<float> pMaster        {0.80f};

    std::atomic<float> pReverbMix     {0.25f};
    std::atomic<float> pReverbSize    {0.50f};
    std::atomic<float> pReverbDamp    {0.50f};
    std::atomic<float> pReverbWidth   {1.00f};

    // Atomic params — tuner + backing track
    std::atomic<bool>  tunerActive    {false};
    std::atomic<float> pBackingVolume {0.80f};
    std::atomic<bool>  btLoop         {false};

    std::atomic<bool>  paramsChanged  {false};
    std::atomic<bool>  dspReady       {false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
