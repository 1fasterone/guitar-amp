#include "MainComponent.h"

//==============================================================================
// Colour palette

static const juce::Colour kBackground    { 0xff1a1a1c };

// Distortion — fire / crimson
static const juce::Colour kDistPanel     { 0xff261414 };
static const juce::Colour kDistAccent    { 0xffdd4400 };
static const juce::Colour kDistAccentDim { 0xff993300 };
static const juce::Colour kDistKnob      { 0xff3a2020 };

// Tape Delay — warm olive / brown (vintage tape machine aesthetic)
static const juce::Colour kDelayPanel    { 0xff1e1c10 };
static const juce::Colour kDelayAccent   { 0xffaa9933 };
static const juce::Colour kDelayAccentDim{ 0xff776622 };
static const juce::Colour kDelayKnob     { 0xff302c18 };

// Amp preamp — charcoal / amber
static const juce::Colour kAmpPanel      { 0xff222226 };
static const juce::Colour kAmpAccent     { 0xffcc8800 };
static const juce::Colour kAmpAccentDim  { 0xff886600 };
static const juce::Colour kAmpKnob       { 0xff36363a };

// Amp power
static const juce::Colour kPowerPanel    { 0xff1e1e22 };

// Reverb — steel blue
static const juce::Colour kRevPanel      { 0xff162030 };
static const juce::Colour kRevAccent     { 0xff5599dd };
static const juce::Colour kRevAccentDim  { 0xff336699 };
static const juce::Colour kRevKnob       { 0xff223040 };

static const juce::Colour kSeparator     { 0xff3a3a3e };

//==============================================================================
// Rotary knob LookAndFeel — one instance per visual section
struct SectionLAF : public juce::LookAndFeel_V4
{
    juce::Colour accent { juce::Colours::orange };
    juce::Colour fill   { juce::Colour(0xff363636) };

    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPos,
                          float startAngle, float endAngle,
                          juce::Slider&) override
    {
        const float r  = (float)juce::jmin(width / 2, height / 2) - 4.0f;
        const float cx = (float)x + width  * 0.5f;
        const float cy = (float)y + height * 0.5f;

        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.fillEllipse(cx - r + 2, cy - r + 2, r * 2, r * 2);

        g.setColour(fill);
        g.fillEllipse(cx - r, cy - r, r * 2, r * 2);

        g.setColour(accent.darker(0.55f));
        g.drawEllipse(cx - r, cy - r, r * 2, r * 2, 1.5f);

        const float angle  = startAngle + sliderPos * (endAngle - startAngle);
        juce::Path ptr;
        ptr.addRectangle(-1.4f, -r, 2.8f, r * 0.58f);
        ptr.applyTransform(juce::AffineTransform::rotation(angle).translated(cx, cy));
        g.setColour(accent);
        g.fillPath(ptr);

        g.setColour(accent.darker(0.6f));
        g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
    }
};

static SectionLAF gDistLAF, gDelayLAF, gAmpLAF, gRevLAF;

//==============================================================================
MainComponent::MainComponent()
{
    gDistLAF.accent  = kDistAccent;   gDistLAF.fill  = kDistKnob;
    gDelayLAF.accent = kDelayAccent;  gDelayLAF.fill = kDelayKnob;
    gAmpLAF.accent   = kAmpAccent;    gAmpLAF.fill   = kAmpKnob;
    gRevLAF.accent   = kRevAccent;    gRevLAF.fill   = kRevKnob;

    auto makeSectionLabel = [&](juce::Label& lbl, const juce::String& text,
                                juce::Colour col)
    {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 9.5f, juce::Font::bold));
        lbl.setColour(juce::Label::textColourId, col);
        lbl.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(lbl);
    };

    // ---- Distortion ---------------------------------------------------------
    setupKnob(driveKnob,     driveLabel,     "DRIVE",    0.50, kDistAccent);
    setupKnob(toneKnob,      toneLabel,      "TONE",     0.50, kDistAccent);
    setupKnob(tightKnob,     tightLabel,     "TIGHT",    0.30, kDistAccent);
    setupKnob(distLevelKnob, distLevelLabel, "DIST LVL", 0.70, kDistAccent);
    makeSectionLabel(distSectionLabel, "POWER METAL DISTORTION", kDistAccentDim);

    // ---- Tape Delay ---------------------------------------------------------
    setupKnob(delayTimeKnob,     delayTimeLabel,     "TIME",     0.35, kDelayAccent);
    setupKnob(delayFeedbackKnob, delayFeedbackLabel, "FEEDBACK", 0.40, kDelayAccent);
    setupKnob(delayMixKnob,      delayMixLabel,      "MIX",      0.30, kDelayAccent);
    setupKnob(delayWowKnob,      delayWowLabel,       "WOW",      0.25, kDelayAccent);
    makeSectionLabel(delaySectionLabel, "TAPE ECHO", kDelayAccentDim);

    // ---- Amp preamp ---------------------------------------------------------
    setupKnob(gainKnob,   gainLabel,   "GAIN",   0.50, kAmpAccent);
    setupKnob(bassKnob,   bassLabel,   "BASS",   0.50, kAmpAccent);
    setupKnob(midKnob,    midLabel,    "MID",    0.50, kAmpAccent);
    setupKnob(trebleKnob, trebleLabel, "TREBLE", 0.50, kAmpAccent);
    makeSectionLabel(preampSectionLabel, "PREAMP", kAmpAccentDim);

    // ---- Amp power ----------------------------------------------------------
    setupKnob(presenceKnob, presenceLabel, "PRESENCE", 0.50, kAmpAccent);
    setupKnob(masterKnob,   masterLabel,   "MASTER",   0.80, kAmpAccent);
    makeSectionLabel(powerAmpSectionLabel, "POWER AMP", kAmpAccentDim);

    // ---- Reverb -------------------------------------------------------------
    setupKnob(reverbMixKnob,   reverbMixLabel,   "MIX",   0.25, kRevAccent);
    setupKnob(reverbSizeKnob,  reverbSizeLabel,  "SIZE",  0.50, kRevAccent);
    setupKnob(reverbDampKnob,  reverbDampLabel,  "DAMP",  0.50, kRevAccent);
    setupKnob(reverbWidthKnob, reverbWidthLabel, "WIDTH", 1.00, kRevAccent);
    makeSectionLabel(reverbSectionLabel, "REVERB", kRevAccentDim);

    // ---- Audio Settings button ----------------------------------------------
    audioSettingsButton.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff2a2a2e));
    audioSettingsButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3a3a3e));
    audioSettingsButton.setColour(juce::TextButton::textColourOffId,  kAmpAccent);
    audioSettingsButton.setColour(juce::TextButton::textColourOnId,   kAmpAccent);
    audioSettingsButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    audioSettingsButton.onClick = [this]
    {
        // JUCE's built-in audio device selector panel — shows all backends,
        // devices, sample rates, buffer sizes and real-time latency readout.
        juce::AudioDeviceSelectorComponent selector(
            deviceManager,
            0, 1,    // min/max input channels
            0, 2,    // min/max output channels
            false,   // show MIDI input
            false,   // show MIDI output
            false,   // stereo pair buttons
            false);  // hide advanced options button

        selector.setSize(500, 320);

        juce::DialogWindow::LaunchOptions dlg;
        dlg.content.setNonOwned(&selector);
        dlg.dialogTitle             = "Audio Device Settings";
        dlg.dialogBackgroundColour  = juce::Colour(0xff1c1c1e);
        dlg.escapeKeyTriggersCloseButton = true;
        dlg.useNativeTitleBar       = true;
        dlg.resizable               = false;
        dlg.launchAsync();
    };

    addAndMakeVisible(audioSettingsButton);

    setSize(900, 530);
    setAudioChannels(1, 2);   // mono in, stereo out
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

//==============================================================================
void MainComponent::setupKnob(juce::Slider& knob, juce::Label& label,
                               const juce::String& name, double defaultVal,
                               juce::Colour accentColour)
{
    SectionLAF* laf = (accentColour == kDistAccent)  ? &gDistLAF
                    : (accentColour == kDelayAccent)  ? &gDelayLAF
                    : (accentColour == kRevAccent)    ? &gRevLAF
                                                      : &gAmpLAF;
    knob.setLookAndFeel(laf);
    knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    knob.setRange(0.0, 1.0);
    knob.setValue(defaultVal, juce::dontSendNotification);
    knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

    // Wire knob → atomic param
    if      (name == "DRIVE")    knob.onValueChange = [this] { pDrive         = (float)driveKnob.getValue();        paramsChanged = true; };
    else if (name == "TONE")     knob.onValueChange = [this] { pTone          = (float)toneKnob.getValue();         paramsChanged = true; };
    else if (name == "TIGHT")    knob.onValueChange = [this] { pTight         = (float)tightKnob.getValue();        paramsChanged = true; };
    else if (name == "DIST LVL") knob.onValueChange = [this] { pDistLevel     = (float)distLevelKnob.getValue();    paramsChanged = true; };
    else if (name == "TIME")     knob.onValueChange = [this] { pDelayTime     = (float)delayTimeKnob.getValue();    paramsChanged = true; };
    else if (name == "FEEDBACK") knob.onValueChange = [this] { pDelayFeedback = (float)delayFeedbackKnob.getValue();paramsChanged = true; };
    else if (name == "MIX" && accentColour == kDelayAccent)
                                 knob.onValueChange = [this] { pDelayMix      = (float)delayMixKnob.getValue();     paramsChanged = true; };
    else if (name == "WOW")      knob.onValueChange = [this] { pDelayWow      = (float)delayWowKnob.getValue();     paramsChanged = true; };
    else if (name == "GAIN")     knob.onValueChange = [this] { pGain          = (float)gainKnob.getValue();         paramsChanged = true; };
    else if (name == "BASS")     knob.onValueChange = [this] { pBass          = (float)bassKnob.getValue();         paramsChanged = true; };
    else if (name == "MID")      knob.onValueChange = [this] { pMid           = (float)midKnob.getValue();          paramsChanged = true; };
    else if (name == "TREBLE")   knob.onValueChange = [this] { pTreble        = (float)trebleKnob.getValue();       paramsChanged = true; };
    else if (name == "PRESENCE") knob.onValueChange = [this] { pPresence      = (float)presenceKnob.getValue();     paramsChanged = true; };
    else if (name == "MASTER")   knob.onValueChange = [this] { pMaster        = (float)masterKnob.getValue();       paramsChanged = true; };
    else if (name == "MIX" && accentColour == kRevAccent)
                                 knob.onValueChange = [this] { pReverbMix     = (float)reverbMixKnob.getValue();    paramsChanged = true; };
    else if (name == "SIZE")     knob.onValueChange = [this] { pReverbSize    = (float)reverbSizeKnob.getValue();   paramsChanged = true; };
    else if (name == "DAMP")     knob.onValueChange = [this] { pReverbDamp    = (float)reverbDampKnob.getValue();   paramsChanged = true; };
    else if (name == "WIDTH")    knob.onValueChange = [this] { pReverbWidth   = (float)reverbWidthKnob.getValue();  paramsChanged = true; };

    addAndMakeVisible(knob);

    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, accentColour);
    addAndMakeVisible(label);
}

//==============================================================================
void MainComponent::prepareToPlay(int /*samplesPerBlockExpected*/, double sampleRate)
{
    powerMetalDist.prepare(sampleRate);
    toneStack.prepare(sampleRate);
    presenceFilter.prepare(sampleRate);
    tapeDelay.prepare(sampleRate);
    reverb.prepare(sampleRate);

    // Push initial values to DSP (audio thread)
    powerMetalDist.setDrive   (pDrive.load());
    powerMetalDist.setTone    (pTone.load());
    powerMetalDist.setTight   (pTight.load());
    powerMetalDist.setLevel   (pDistLevel.load());
    tapeDelay.setTime         (pDelayTime.load());
    tapeDelay.setFeedback     (pDelayFeedback.load());
    tapeDelay.setMix          (pDelayMix.load());
    tapeDelay.setWow          (pDelayWow.load());
    gainStage.setGain          (pGain.load());
    toneStack.setBass          (pBass.load());
    toneStack.setMid           (pMid.load());
    toneStack.setTreble        (pTreble.load());
    presenceFilter.setPresence (pPresence.load());
    masterVolume.setMaster     (pMaster.load());
    reverb.setMix  (pReverbMix.load());
    reverb.setSize (pReverbSize.load());
    reverb.setDamp (pReverbDamp.load());
    reverb.setWidth(pReverbWidth.load());

    dspReady = true;
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& info)
{
    if (!dspReady) { info.clearActiveBufferRegion(); return; }

    // Absorb knob changes from UI thread (lock-free, once per block)
    if (paramsChanged.exchange(false))
    {
        powerMetalDist.setDrive   (pDrive.load());
        powerMetalDist.setTone    (pTone.load());
        powerMetalDist.setTight   (pTight.load());
        powerMetalDist.setLevel   (pDistLevel.load());
        tapeDelay.setTime         (pDelayTime.load());
        tapeDelay.setFeedback     (pDelayFeedback.load());
        tapeDelay.setMix          (pDelayMix.load());
        tapeDelay.setWow          (pDelayWow.load());
        gainStage.setGain          (pGain.load());
        toneStack.setBass          (pBass.load());
        toneStack.setMid           (pMid.load());
        toneStack.setTreble        (pTreble.load());
        presenceFilter.setPresence (pPresence.load());
        masterVolume.setMaster     (pMaster.load());
        reverb.setMix  (pReverbMix.load());
        reverb.setSize (pReverbSize.load());
        reverb.setDamp (pReverbDamp.load());
        reverb.setWidth(pReverbWidth.load());
    }

    // =========================================================================
    // Signal chain — sample by sample
    // =========================================================================
    const bool hasStereo = (info.buffer->getNumChannels() >= 2);
    float* outL = info.buffer->getWritePointer(0, info.startSample);
    float* outR = hasStereo ? info.buffer->getWritePointer(1, info.startSample)
                            : nullptr;

    for (int i = 0; i < info.numSamples; ++i)
    {
        const float inputSample = outL[i];   // save before overwriting
        double s = (double)inputSample;

        s = powerMetalDist.process(s);          // distortion pedal
        s = gainStage.process((float)s);        // amp preamp drive
        s = toneStack.process(s);               // Bass / Mid / Treble
        s = presenceFilter.process(s);          // power-amp presence
        s = tapeDelay.process((float)s);        // tape echo

        float reverbL, reverbR;
        reverb.process((float)s, reverbL, reverbR);

        outL[i] = masterVolume.process(reverbL);
        if (outR != nullptr)
            outR[i] = masterVolume.process(reverbR);
    }
}

void MainComponent::releaseResources()
{
    dspReady = false;
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(kBackground);

    const int m       = 6;
    const int rowH    = 160;    // height of each FX row
    const int row1Y   = m;
    const int row2Y   = row1Y + rowH + m;
    const int row3Y   = row2Y + rowH + m;

    // x dividers for bottom (amp) row
    const int preampEndX = (int)(getWidth() * 0.43f);
    const int powerEndX  = (int)(getWidth() * 0.60f);

    auto drawPanel = [&](juce::Colour fillCol, juce::Colour rimCol,
                         int x, int y, int w, int h,
                         bool gradientTop = false)
    {
        auto r = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h);
        g.setColour(fillCol);
        g.fillRoundedRectangle(r, 8.0f);
        if (gradientTop)
        {
            juce::ColourGradient gr(rimCol.withAlpha(0.06f), 0, (float)y,
                                    juce::Colours::transparentBlack, 0, (float)(y + 50), false);
            g.setGradientFill(gr);
            g.fillRoundedRectangle(r, 8.0f);
        }
        g.setColour(rimCol);
        g.drawRoundedRectangle(r, 8.0f, 1.2f);
    };

    // Row 1: Distortion
    drawPanel(kDistPanel,  kDistAccent.darker(0.7f),
              m, row1Y, getWidth() - m * 2, rowH, true);

    // Row 2: Tape Delay
    drawPanel(kDelayPanel, kDelayAccent.darker(0.6f),
              m, row2Y, getWidth() - m * 2, rowH, true);

    // Row 3: three sub-panels (Preamp | Power Amp | Reverb)
    drawPanel(kAmpPanel,   kSeparator,
              m, row3Y, preampEndX - m * 2, rowH);

    drawPanel(kPowerPanel, kSeparator,
              preampEndX + m, row3Y, powerEndX - preampEndX - m, rowH);

    drawPanel(kRevPanel,   kRevAccent.darker(0.7f),
              powerEndX + m, row3Y, getWidth() - powerEndX - m * 2, rowH);

    // Title bar
    g.setColour(kAmpAccent);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 11.5f, juce::Font::bold));
    g.drawText("GUITAR AMP MODELER",
               juce::Rectangle<int>(0, 0, getWidth(), 20),
               juce::Justification::centred);

    // Thin separator below title bar
    g.setColour(kSeparator);
    g.drawHorizontalLine(21, (float)m, (float)(getWidth() - m));
}

void MainComponent::resized()
{
    const int m        = 10;
    const int titleH   = 22;
    const int labelH   = 20;
    const int secH     = 18;

    // Audio settings button — top-right of title bar
    audioSettingsButton.setBounds(getWidth() - 160 - m, 1, 160, titleH - 2);
    const int rowH     = 160;
    const int row1Y    = m + titleH;
    const int row2Y    = m + rowH + m + titleH;
    const int row3Y    = m + rowH + m + rowH + m + titleH;

    const int preampEndX = (int)(getWidth() * 0.43f);
    const int powerEndX  = (int)(getWidth() * 0.60f);

    // Utility: carve one knob slot from the left of area
    auto placeKnob = [&](juce::Rectangle<int>& area, int w,
                         juce::Slider& knob, juce::Label& lbl)
    {
        auto col = area.removeFromLeft(w);
        lbl.setBounds(col.removeFromBottom(labelH));
        knob.setBounds(col.reduced(5, 2));
    };

    // =========================================================================
    // Row 1 — Distortion
    // =========================================================================
    {
        auto area = juce::Rectangle<int>(m * 2, row1Y,
                                         getWidth() - m * 4,
                                         rowH - m - titleH);
        distSectionLabel.setBounds(area.removeFromTop(secH));
        const int kw = area.getWidth() / 4;
        placeKnob(area, kw, driveKnob,     driveLabel);
        placeKnob(area, kw, toneKnob,      toneLabel);
        placeKnob(area, kw, tightKnob,     tightLabel);
        placeKnob(area, kw, distLevelKnob, distLevelLabel);
    }

    // =========================================================================
    // Row 2 — Tape Delay
    // =========================================================================
    {
        auto area = juce::Rectangle<int>(m * 2, row2Y,
                                         getWidth() - m * 4,
                                         rowH - m - titleH);
        delaySectionLabel.setBounds(area.removeFromTop(secH));
        const int kw = area.getWidth() / 4;
        placeKnob(area, kw, delayTimeKnob,     delayTimeLabel);
        placeKnob(area, kw, delayFeedbackKnob, delayFeedbackLabel);
        placeKnob(area, kw, delayMixKnob,      delayMixLabel);
        placeKnob(area, kw, delayWowKnob,      delayWowLabel);
    }

    // =========================================================================
    // Row 3 — Amp + Reverb (three sub-panels)
    // =========================================================================
    const int botContentH = rowH - m - titleH;

    // Preamp
    {
        auto area = juce::Rectangle<int>(m * 2, row3Y,
                                         preampEndX - m * 3, botContentH);
        preampSectionLabel.setBounds(area.removeFromTop(secH));
        const int kw = area.getWidth() / 4;
        placeKnob(area, kw, gainKnob,   gainLabel);
        placeKnob(area, kw, bassKnob,   bassLabel);
        placeKnob(area, kw, midKnob,    midLabel);
        placeKnob(area, kw, trebleKnob, trebleLabel);
    }

    // Power amp
    {
        auto area = juce::Rectangle<int>(preampEndX + m, row3Y,
                                         powerEndX - preampEndX - m, botContentH);
        powerAmpSectionLabel.setBounds(area.removeFromTop(secH));
        const int kw = area.getWidth() / 2;
        placeKnob(area, kw, presenceKnob, presenceLabel);
        placeKnob(area, kw, masterKnob,   masterLabel);
    }

    // Reverb
    {
        auto area = juce::Rectangle<int>(powerEndX + m, row3Y,
                                         getWidth() - powerEndX - m * 2, botContentH);
        reverbSectionLabel.setBounds(area.removeFromTop(secH));
        const int kw = area.getWidth() / 4;
        placeKnob(area, kw, reverbMixKnob,   reverbMixLabel);
        placeKnob(area, kw, reverbSizeKnob,  reverbSizeLabel);
        placeKnob(area, kw, reverbDampKnob,  reverbDampLabel);
        placeKnob(area, kw, reverbWidthKnob, reverbWidthLabel);
    }
}
