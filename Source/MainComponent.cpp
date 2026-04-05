#include "MainComponent.h"

//==============================================================================
static const juce::Colour kBackground    { 0xff1a1a1c };
static const juce::Colour kSeparator     { 0xff3a3a3e };
static const juce::Colour kValueText     { 0xffaaaaaa };

// Noise Gate — dark green
static const juce::Colour kGatePanel     { 0xff0e1a0e };
static const juce::Colour kGateAccent    { 0xff44cc44 };
static const juce::Colour kGateAccentDim { 0xff2a882a };
static const juce::Colour kGateKnob      { 0xff1a2a1a };

// Distortion — crimson / fire
static const juce::Colour kDistPanel     { 0xff261414 };
static const juce::Colour kDistAccent    { 0xffdd4400 };
static const juce::Colour kDistAccentDim { 0xff993300 };
static const juce::Colour kDistKnob      { 0xff3a2020 };

// Phaser — purple
static const juce::Colour kPhasePanel    { 0xff1a1428 };
static const juce::Colour kPhaseAccent   { 0xffaa66ff };
static const juce::Colour kPhaseAccentDim{ 0xff7744bb };
static const juce::Colour kPhaseKnob     { 0xff2a2038 };

// Tape Delay — olive
static const juce::Colour kDelayPanel    { 0xff1e1c10 };
static const juce::Colour kDelayAccent   { 0xffaa9933 };
static const juce::Colour kDelayAccentDim{ 0xff776622 };
static const juce::Colour kDelayKnob     { 0xff302c18 };

// Amp — amber
static const juce::Colour kAmpPanel      { 0xff222226 };
static const juce::Colour kPowerPanel    { 0xff1e1e22 };
static const juce::Colour kAmpAccent     { 0xffcc8800 };
static const juce::Colour kAmpAccentDim  { 0xff886600 };
static const juce::Colour kAmpKnob       { 0xff36363a };

// Reverb — steel blue
static const juce::Colour kRevPanel      { 0xff162030 };
static const juce::Colour kRevAccent     { 0xff5599dd };
static const juce::Colour kRevAccentDim  { 0xff336699 };
static const juce::Colour kRevKnob       { 0xff223040 };

// Tuner — teal / cyan
static const juce::Colour kTunerPanel    { 0xff0d1c1c };
static const juce::Colour kTunerAccent   { 0xff33bbcc };
static const juce::Colour kTunerAccentDim{ 0xff226677 };
static const juce::Colour kTunerKnob     { 0xff1a3030 };

// Backing Track — purple / magenta
static const juce::Colour kBTPanel       { 0xff1a0e22 };
static const juce::Colour kBTAccent      { 0xffcc55ff };
static const juce::Colour kBTAccentDim   { 0xff883399 };
static const juce::Colour kBTKnob        { 0xff2a1838 };

//==============================================================================
struct SectionLAF : public juce::LookAndFeel_V4
{
    juce::Colour accent { juce::Colours::orange };
    juce::Colour fill   { juce::Colour(0xff363636) };

    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override
    {
        const float r  = (float)juce::jmin(width / 2, height / 2) - 4.0f;
        const float cx = (float)x + width  * 0.5f;
        const float cy = (float)y + height * 0.5f;

        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.fillEllipse(cx-r+2, cy-r+2, r*2, r*2);
        g.setColour(fill);
        g.fillEllipse(cx-r, cy-r, r*2, r*2);
        g.setColour(accent.darker(0.55f));
        g.drawEllipse(cx-r, cy-r, r*2, r*2, 1.5f);

        const float angle = startAngle + sliderPos * (endAngle - startAngle);
        juce::Path ptr;
        ptr.addRectangle(-1.4f, -r, 2.8f, r * 0.58f);
        ptr.applyTransform(juce::AffineTransform::rotation(angle).translated(cx, cy));
        g.setColour(accent);
        g.fillPath(ptr);
        g.setColour(accent.darker(0.6f));
        g.fillEllipse(cx-2.5f, cy-2.5f, 5.0f, 5.0f);
    }
};

static SectionLAF gGateLAF, gDistLAF, gPhaseLAF, gDelayLAF, gAmpLAF, gRevLAF, gTunerLAF, gBTLAF;

static juce::Font monoFont(float size, int style = juce::Font::plain)
{
    return juce::Font(juce::Font::getDefaultMonospacedFontName(), size, style);
}

//==============================================================================
MainComponent::MainComponent()
{
    gGateLAF.accent   = kGateAccent;   gGateLAF.fill   = kGateKnob;
    gDistLAF.accent   = kDistAccent;   gDistLAF.fill   = kDistKnob;
    gPhaseLAF.accent  = kPhaseAccent;  gPhaseLAF.fill  = kPhaseKnob;
    gDelayLAF.accent  = kDelayAccent;  gDelayLAF.fill  = kDelayKnob;
    gAmpLAF.accent    = kAmpAccent;    gAmpLAF.fill    = kAmpKnob;
    gRevLAF.accent    = kRevAccent;    gRevLAF.fill    = kRevKnob;
    gTunerLAF.accent  = kTunerAccent;  gTunerLAF.fill  = kTunerKnob;
    gBTLAF.accent     = kBTAccent;     gBTLAF.fill     = kBTKnob;

    formatManager.registerBasicFormats();

    auto makeSectionLabel = [&](juce::Label& lbl, const juce::String& text, juce::Colour col)
    {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setFont(monoFont(11.0f, juce::Font::bold));
        lbl.setColour(juce::Label::textColourId, col);
        lbl.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(lbl);
    };

    // ---- Noise Gate ---------------------------------------------------------
    setupKnob(gateThreshKnob,  gateThreshLabel,  gateThreshValLabel,  "THRESHOLD", 0.30, kGateAccent);
    setupKnob(gateAttackKnob,  gateAttackLabel,  gateAttackValLabel,  "ATTACK",    0.20, kGateAccent);
    setupKnob(gateReleaseKnob, gateReleaseLabel, gateReleaseValLabel, "RELEASE",   0.50, kGateAccent);
    setupKnob(gateHoldKnob,    gateHoldLabel,    gateHoldValLabel,    "HOLD",      0.30, kGateAccent);
    makeSectionLabel(gateSectionLabel, ">>  NOISE GATE", kGateAccentDim);

    // ---- Distortion ---------------------------------------------------------
    setupKnob(driveKnob,     driveLabel,     driveValLabel,     "DRIVE", 0.50, kDistAccent);
    setupKnob(toneKnob,      toneLabel,      toneValLabel,      "TONE",  0.50, kDistAccent);
    setupKnob(tightKnob,     tightLabel,     tightValLabel,     "TIGHT", 0.30, kDistAccent);
    setupKnob(distLevelKnob, distLevelLabel, distLevelValLabel, "LEVEL", 0.70, kDistAccent);
    makeSectionLabel(distSectionLabel, ">>  POWER METAL DISTORTION", kDistAccentDim);

    // ---- Phaser -------------------------------------------------------------
    setupKnob(phaserRateKnob,     phaserRateLabel,     phaserRateValLabel,     "RATE",     0.30, kPhaseAccent);
    setupKnob(phaserDepthKnob,    phaserDepthLabel,    phaserDepthValLabel,    "DEPTH",    0.70, kPhaseAccent);
    setupKnob(phaserFeedbackKnob, phaserFeedbackLabel, phaserFeedbackValLabel, "FEEDBACK", 0.40, kPhaseAccent);
    setupKnob(phaserMixKnob,      phaserMixLabel,      phaserMixValLabel,      "MIX",      0.50, kPhaseAccent);
    makeSectionLabel(phaserSectionLabel, ">>  PHASER", kPhaseAccentDim);

    // ---- Tape Delay ---------------------------------------------------------
    setupKnob(delayTimeKnob,     delayTimeLabel,     delayTimeValLabel,     "TIME",     0.35, kDelayAccent);
    setupKnob(delayFeedbackKnob, delayFeedbackLabel, delayFeedbackValLabel, "FEEDBACK", 0.40, kDelayAccent);
    setupKnob(delayMixKnob,      delayMixLabel,      delayMixValLabel,      "MIX",      0.30, kDelayAccent);
    setupKnob(delayWowKnob,      delayWowLabel,      delayWowValLabel,      "WOW",      0.25, kDelayAccent);
    makeSectionLabel(delaySectionLabel, ">>  TAPE ECHO", kDelayAccentDim);

    // ---- Amp preamp ---------------------------------------------------------
    setupKnob(gainKnob,   gainLabel,   gainValLabel,   "GAIN",   0.50, kAmpAccent);
    setupKnob(bassKnob,   bassLabel,   bassValLabel,   "BASS",   0.50, kAmpAccent);
    setupKnob(midKnob,    midLabel,    midValLabel,    "MID",    0.50, kAmpAccent);
    setupKnob(trebleKnob, trebleLabel, trebleValLabel, "TREBLE", 0.50, kAmpAccent);
    makeSectionLabel(preampSectionLabel, ">>  PREAMP", kAmpAccentDim);

    // ---- Amp power ----------------------------------------------------------
    setupKnob(presenceKnob, presenceLabel, presenceValLabel, "PRESENCE", 0.50, kAmpAccent);
    setupKnob(masterKnob,   masterLabel,   masterValLabel,   "MASTER",   0.80, kAmpAccent);
    makeSectionLabel(powerAmpSectionLabel, ">>  POWER AMP", kAmpAccentDim);

    // ---- Reverb -------------------------------------------------------------
    setupKnob(reverbMixKnob,   reverbMixLabel,   reverbMixValLabel,   "MIX",   0.25, kRevAccent);
    setupKnob(reverbSizeKnob,  reverbSizeLabel,  reverbSizeValLabel,  "SIZE",  0.50, kRevAccent);
    setupKnob(reverbDampKnob,  reverbDampLabel,  reverbDampValLabel,  "DAMP",  0.50, kRevAccent);
    setupKnob(reverbWidthKnob, reverbWidthLabel, reverbWidthValLabel, "WIDTH", 1.00, kRevAccent);
    makeSectionLabel(reverbSectionLabel, ">>  REVERB", kRevAccentDim);

    // ---- Tuner --------------------------------------------------------------
    makeSectionLabel(tunerSectionLabel, ">>  CHROMATIC TUNER", kTunerAccentDim);

    tunerOnButton.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff1a3030));
    tunerOnButton.setColour(juce::TextButton::textColourOffId,  kTunerAccent);
    tunerOnButton.setColour(juce::TextButton::buttonOnColourId, kTunerAccent);
    tunerOnButton.setColour(juce::TextButton::textColourOnId,   juce::Colours::black);
    tunerOnButton.setClickingTogglesState(true);
    tunerOnButton.onClick = [this]
    {
        tunerActive = tunerOnButton.getToggleState();
        tunerOnButton.setButtonText(tunerActive.load() ? "TUNER: ON" : "TUNER: OFF");
    };
    addAndMakeVisible(tunerOnButton);

    auto styleLabel = [&](juce::Label& lbl, float size, int style, juce::Colour col,
                          juce::Justification just = juce::Justification::centred)
    {
        lbl.setFont(monoFont(size, style));
        lbl.setColour(juce::Label::textColourId, col);
        lbl.setJustificationType(just);
        addAndMakeVisible(lbl);
    };

    tunerNoteLabel.setText  ("--",  juce::dontSendNotification);
    tunerCentsLabel.setText ("",    juce::dontSendNotification);
    tunerFreqLabel.setText  ("",    juce::dontSendNotification);
    styleLabel(tunerNoteLabel,  40.0f, juce::Font::bold,  kTunerAccent);
    styleLabel(tunerCentsLabel, 14.0f, juce::Font::plain, kValueText);
    styleLabel(tunerFreqLabel,  11.0f, juce::Font::plain, kValueText);

    // ---- Backing Track ------------------------------------------------------
    makeSectionLabel(btSectionLabel, ">>  BACKING TRACK", kBTAccentDim);

    auto styleBtn = [&](juce::TextButton& btn, juce::Colour bg, juce::Colour fg)
    {
        btn.setColour(juce::TextButton::buttonColourId,  bg);
        btn.setColour(juce::TextButton::textColourOffId, fg);
        btn.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        addAndMakeVisible(btn);
    };

    styleBtn(btLoadButton, juce::Colour(0xff251530), kBTAccent);
    btLoadButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Load Backing Track",
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            "*.mp3;*.wav;*.aiff;*.flac;*.ogg");

        fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode |
            juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                auto result = fc.getResult();
                if (!result.existsAsFile()) return;

                auto* reader = formatManager.createReaderFor(result);
                if (reader == nullptr) return;

                auto newSrc = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
                transportSource.stop();
                transportSource.setSource(newSrc.get(), 0, nullptr, reader->sampleRate);
                readerSource = std::move(newSrc);

                juce::MessageManager::callAsync([this, name = result.getFileName()]
                {
                    btFileLabel.setText(name, juce::dontSendNotification);
                    btPlayButton.setButtonText("PLAY");
                });
            });
    };

    styleBtn(btPlayButton, juce::Colour(0xff1a2515), kBTAccent);
    btPlayButton.onClick = [this]
    {
        if (transportSource.isPlaying())
        {
            transportSource.stop();
        }
        else if (readerSource != nullptr)
        {
            // If we've reached the end, rewind
            if (transportSource.getLengthInSeconds() > 0.0 &&
                transportSource.getCurrentPosition() >=
                    transportSource.getLengthInSeconds() - 0.05)
            {
                transportSource.setPosition(0.0);
            }
            transportSource.start();
        }
    };

    btLoopButton.setClickingTogglesState(true);
    btLoopButton.setColour(juce::TextButton::buttonColourId,    juce::Colour(0xff201520));
    btLoopButton.setColour(juce::TextButton::textColourOffId,   kBTAccent);
    btLoopButton.setColour(juce::TextButton::buttonOnColourId,  kBTAccent);
    btLoopButton.setColour(juce::TextButton::textColourOnId,    juce::Colours::black);
    btLoopButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    btLoopButton.onClick = [this]
    {
        bool loop = btLoopButton.getToggleState();
        btLoop = loop;
        btLoopButton.setButtonText(loop ? "LOOP: ON" : "LOOP: OFF");
        if (readerSource != nullptr)
            readerSource->setLooping(loop);
    };
    addAndMakeVisible(btLoopButton);

    // Volume knob (manually set up — doesn't go through setupKnob)
    btVolumeKnob.setLookAndFeel(&gBTLAF);
    btVolumeKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    btVolumeKnob.setRange(0.0, 1.0);
    btVolumeKnob.setValue(0.80, juce::dontSendNotification);
    btVolumeKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    btVolumeKnob.onValueChange = [this] { pBackingVolume = (float)btVolumeKnob.getValue(); };
    addAndMakeVisible(btVolumeKnob);

    btVolumeLabel.setText("VOLUME", juce::dontSendNotification);
    styleLabel(btVolumeLabel,  13.0f, juce::Font::bold,  kBTAccent);
    btVolumeValLabel.setText("80%", juce::dontSendNotification);
    styleLabel(btVolumeValLabel, 11.0f, juce::Font::plain, kValueText);

    btFileLabel.setText("No file loaded", juce::dontSendNotification);
    styleLabel(btFileLabel, 11.0f, juce::Font::plain, kValueText,
               juce::Justification::centredLeft);

    // ---- Audio settings button ----------------------------------------------
    audioSettingsButton.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff2a2a2e));
    audioSettingsButton.setColour(juce::TextButton::textColourOffId, kAmpAccent);
    audioSettingsButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    audioSettingsButton.onClick = [this]
    {
        // Must use setOwned + new — launchAsync() is non-blocking so any
        // stack-allocated component would be destroyed before the dialog renders.
        auto* selector = new juce::AudioDeviceSelectorComponent(
            deviceManager, 0, 1, 0, 2, false, false, false, false);
        selector->setSize(500, 370);

        juce::DialogWindow::LaunchOptions dlg;
        dlg.content.setOwned(selector);          // dialog now owns the lifetime
        dlg.dialogTitle            = "Audio Device Settings";
        dlg.dialogBackgroundColour = juce::Colour(0xff1c1c1e);
        dlg.escapeKeyTriggersCloseButton = true;
        dlg.useNativeTitleBar      = true;
        dlg.resizable              = false;
        dlg.launchAsync();
    };
    addAndMakeVisible(audioSettingsButton);

    startTimer(66);   // 15 fps value-label refresh
    setSize(1000, 985);
    setAudioChannels(1, 2);
}

MainComponent::~MainComponent()
{
    stopTimer();
    transportSource.setSource(nullptr);   // disconnect before audio shuts down
    shutdownAudio();
}

//==============================================================================
void MainComponent::setupKnob(juce::Slider& knob, juce::Label& nameLabel,
                               juce::Label& valueLabel,
                               const juce::String& name, double defaultVal,
                               juce::Colour accentColour)
{
    SectionLAF* laf = (accentColour == kGateAccent)  ? &gGateLAF
                    : (accentColour == kDistAccent)   ? &gDistLAF
                    : (accentColour == kPhaseAccent)  ? &gPhaseLAF
                    : (accentColour == kDelayAccent)  ? &gDelayLAF
                    : (accentColour == kRevAccent)    ? &gRevLAF
                                                      : &gAmpLAF;

    knob.setLookAndFeel(laf);
    knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    knob.setRange(0.0, 1.0);
    knob.setValue(defaultVal, juce::dontSendNotification);
    knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

    if      (name == "THRESHOLD") knob.onValueChange = [this] { pGateThresh    = (float)gateThreshKnob.getValue();     paramsChanged = true; };
    else if (name == "ATTACK")    knob.onValueChange = [this] { pGateAttack    = (float)gateAttackKnob.getValue();     paramsChanged = true; };
    else if (name == "RELEASE")   knob.onValueChange = [this] { pGateRelease   = (float)gateReleaseKnob.getValue();    paramsChanged = true; };
    else if (name == "HOLD")      knob.onValueChange = [this] { pGateHold      = (float)gateHoldKnob.getValue();       paramsChanged = true; };
    else if (name == "DRIVE")     knob.onValueChange = [this] { pDrive         = (float)driveKnob.getValue();          paramsChanged = true; };
    else if (name == "TONE")      knob.onValueChange = [this] { pTone          = (float)toneKnob.getValue();           paramsChanged = true; };
    else if (name == "TIGHT")     knob.onValueChange = [this] { pTight         = (float)tightKnob.getValue();          paramsChanged = true; };
    else if (name == "LEVEL")     knob.onValueChange = [this] { pDistLevel     = (float)distLevelKnob.getValue();      paramsChanged = true; };
    else if (name == "RATE")      knob.onValueChange = [this] { pPhaserRate    = (float)phaserRateKnob.getValue();     paramsChanged = true; };
    else if (name == "DEPTH")     knob.onValueChange = [this] { pPhaserDepth   = (float)phaserDepthKnob.getValue();    paramsChanged = true; };
    else if (name == "FEEDBACK" && accentColour == kPhaseAccent)
                                  knob.onValueChange = [this] { pPhaserFeedback= (float)phaserFeedbackKnob.getValue(); paramsChanged = true; };
    else if (name == "MIX" && accentColour == kPhaseAccent)
                                  knob.onValueChange = [this] { pPhaserMix     = (float)phaserMixKnob.getValue();      paramsChanged = true; };
    else if (name == "TIME")      knob.onValueChange = [this] { pDelayTime     = (float)delayTimeKnob.getValue();      paramsChanged = true; };
    else if (name == "FEEDBACK" && accentColour == kDelayAccent)
                                  knob.onValueChange = [this] { pDelayFeedback = (float)delayFeedbackKnob.getValue();  paramsChanged = true; };
    else if (name == "MIX" && accentColour == kDelayAccent)
                                  knob.onValueChange = [this] { pDelayMix      = (float)delayMixKnob.getValue();       paramsChanged = true; };
    else if (name == "WOW")       knob.onValueChange = [this] { pDelayWow      = (float)delayWowKnob.getValue();       paramsChanged = true; };
    else if (name == "GAIN")      knob.onValueChange = [this] { pGain          = (float)gainKnob.getValue();           paramsChanged = true; };
    else if (name == "BASS")      knob.onValueChange = [this] { pBass          = (float)bassKnob.getValue();           paramsChanged = true; };
    else if (name == "MID")       knob.onValueChange = [this] { pMid           = (float)midKnob.getValue();            paramsChanged = true; };
    else if (name == "TREBLE")    knob.onValueChange = [this] { pTreble        = (float)trebleKnob.getValue();         paramsChanged = true; };
    else if (name == "PRESENCE")  knob.onValueChange = [this] { pPresence      = (float)presenceKnob.getValue();       paramsChanged = true; };
    else if (name == "MASTER")    knob.onValueChange = [this] { pMaster        = (float)masterKnob.getValue();         paramsChanged = true; };
    else if (name == "MIX" && accentColour == kRevAccent)
                                  knob.onValueChange = [this] { pReverbMix     = (float)reverbMixKnob.getValue();      paramsChanged = true; };
    else if (name == "SIZE")      knob.onValueChange = [this] { pReverbSize    = (float)reverbSizeKnob.getValue();     paramsChanged = true; };
    else if (name == "DAMP")      knob.onValueChange = [this] { pReverbDamp    = (float)reverbDampKnob.getValue();     paramsChanged = true; };
    else if (name == "WIDTH")     knob.onValueChange = [this] { pReverbWidth   = (float)reverbWidthKnob.getValue();    paramsChanged = true; };

    addAndMakeVisible(knob);

    nameLabel.setText(name, juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setFont(monoFont(13.0f, juce::Font::bold));
    nameLabel.setColour(juce::Label::textColourId, accentColour);
    addAndMakeVisible(nameLabel);

    valueLabel.setText(formatValue(name, defaultVal), juce::dontSendNotification);
    valueLabel.setJustificationType(juce::Justification::centred);
    valueLabel.setFont(monoFont(11.0f));
    valueLabel.setColour(juce::Label::textColourId, kValueText);
    addAndMakeVisible(valueLabel);
}

//==============================================================================
juce::String MainComponent::formatValue(const juce::String& name, double v)
{
    if (name == "THRESHOLD")
    {
        double db = -80.0 + v * 60.0;
        return juce::String((int)db) + " dBFS";
    }
    if (name == "ATTACK")
    {
        double ms = 0.1 * std::pow(500.0, v);
        return ms < 10.0 ? juce::String(ms, 1) + " ms"
                         : juce::String((int)ms) + " ms";
    }
    if (name == "RELEASE")
    {
        double ms = 10.0 * std::pow(50.0, v);
        return juce::String((int)ms) + " ms";
    }
    if (name == "HOLD")
    {
        int ms = (int)(v * 500.0);
        return juce::String(ms) + " ms";
    }
    if (name == "TIME")
    {
        double ms = 1.0 * std::pow(600.0, v);
        return juce::String((int)ms) + " ms";
    }
    if (name == "RATE")
    {
        double hz = 0.1 * std::pow(80.0, v);
        return juce::String(hz, 1) + " Hz";
    }
    if (name == "BASS" || name == "MID" || name == "TREBLE" || name == "PRESENCE")
    {
        double db = (v - 0.5) * 24.0;
        return (db >= 0 ? "+" : "") + juce::String(db, 1) + " dB";
    }
    return juce::String((int)(v * 100.0)) + "%";
}

//==============================================================================
void MainComponent::timerCallback()
{
    struct Pair { juce::Slider* k; juce::Label* l; juce::String n; };
    Pair pairs[] = {
        { &gateThreshKnob,    &gateThreshValLabel,    "THRESHOLD" },
        { &gateAttackKnob,    &gateAttackValLabel,    "ATTACK"    },
        { &gateReleaseKnob,   &gateReleaseValLabel,   "RELEASE"   },
        { &gateHoldKnob,      &gateHoldValLabel,      "HOLD"      },
        { &driveKnob,         &driveValLabel,         "DRIVE"     },
        { &toneKnob,          &toneValLabel,          "TONE"      },
        { &tightKnob,         &tightValLabel,         "TIGHT"     },
        { &distLevelKnob,     &distLevelValLabel,     "LEVEL"     },
        { &phaserRateKnob,    &phaserRateValLabel,    "RATE"      },
        { &phaserDepthKnob,   &phaserDepthValLabel,   "DEPTH"     },
        { &phaserFeedbackKnob,&phaserFeedbackValLabel,"FEEDBACK"  },
        { &phaserMixKnob,     &phaserMixValLabel,     "MIX"       },
        { &delayTimeKnob,     &delayTimeValLabel,     "TIME"      },
        { &delayFeedbackKnob, &delayFeedbackValLabel, "FEEDBACK"  },
        { &delayMixKnob,      &delayMixValLabel,      "MIX"       },
        { &delayWowKnob,      &delayWowValLabel,      "WOW"       },
        { &gainKnob,          &gainValLabel,          "GAIN"      },
        { &bassKnob,          &bassValLabel,          "BASS"      },
        { &midKnob,           &midValLabel,           "MID"       },
        { &trebleKnob,        &trebleValLabel,        "TREBLE"    },
        { &presenceKnob,      &presenceValLabel,      "PRESENCE"  },
        { &masterKnob,        &masterValLabel,        "MASTER"    },
        { &reverbMixKnob,     &reverbMixValLabel,     "MIX"       },
        { &reverbSizeKnob,    &reverbSizeValLabel,    "SIZE"      },
        { &reverbDampKnob,    &reverbDampValLabel,    "DAMP"      },
        { &reverbWidthKnob,   &reverbWidthValLabel,   "WIDTH"     },
    };
    for (auto& p : pairs)
        p.l->setText(formatValue(p.n, p.k->getValue()), juce::dontSendNotification);

    // ---- Tuner display ------------------------------------------------------
    if (tunerActive.load())
    {
        float freq = 0.0f;
        int   note = 0;
        float cents = 0.0f;
        if (pitchDetector.analyze(freq, note, cents))
        {
            juce::String noteStr = juce::String(PitchDetector::noteName(note))
                                   + juce::String(PitchDetector::noteOctave(note));
            tunerNoteLabel.setText(noteStr, juce::dontSendNotification);

            juce::String cStr = (cents >= 0 ? "+" : "")
                                + juce::String((int)std::round(cents)) + " cents";
            tunerCentsLabel.setText(cStr, juce::dontSendNotification);
            tunerFreqLabel.setText(juce::String(freq, 1) + " Hz",
                                   juce::dontSendNotification);

            // Colour the note green when in tune (within ±8 cents), amber otherwise
            bool inTune = std::abs(cents) < 8.0f;
            tunerNoteLabel.setColour(juce::Label::textColourId,
                                     inTune ? juce::Colour(0xff44ee44) : kTunerAccent);
            tunerCentsLabel.setColour(juce::Label::textColourId,
                                      inTune ? juce::Colour(0xff44ee44) : kValueText);
        }
    }
    else
    {
        tunerNoteLabel.setText("--", juce::dontSendNotification);
        tunerNoteLabel.setColour(juce::Label::textColourId, kTunerAccent);
        tunerCentsLabel.setText("", juce::dontSendNotification);
        tunerFreqLabel.setText("",  juce::dontSendNotification);
    }

    // ---- Backing track button state -----------------------------------------
    btPlayButton.setButtonText(transportSource.isPlaying() ? "STOP" : "PLAY");
    btVolumeValLabel.setText(
        juce::String((int)(btVolumeKnob.getValue() * 100.0)) + "%",
        juce::dontSendNotification);
}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    pitchDetector.setSampleRate(sampleRate);
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    backingBuffer.setSize(2, samplesPerBlockExpected + 512, false, true, false);

    noiseGate.prepare(sampleRate);
    powerMetalDist.prepare(sampleRate);
    toneStack.prepare(sampleRate);
    presenceFilter.prepare(sampleRate);
    phaser.prepare(sampleRate);
    tapeDelay.prepare(sampleRate);
    reverb.prepare(sampleRate);

    noiseGate.setThreshold (pGateThresh.load());
    noiseGate.setAttack    (pGateAttack.load());
    noiseGate.setRelease   (pGateRelease.load());
    noiseGate.setHold      (pGateHold.load());
    powerMetalDist.setDrive(pDrive.load());
    powerMetalDist.setTone (pTone.load());
    powerMetalDist.setTight(pTight.load());
    powerMetalDist.setLevel(pDistLevel.load());
    phaser.setRate         (pPhaserRate.load());
    phaser.setDepth        (pPhaserDepth.load());
    phaser.setFeedback     (pPhaserFeedback.load());
    phaser.setMix          (pPhaserMix.load());
    tapeDelay.setTime      (pDelayTime.load());
    tapeDelay.setFeedback  (pDelayFeedback.load());
    tapeDelay.setMix       (pDelayMix.load());
    tapeDelay.setWow       (pDelayWow.load());
    gainStage.setGain       (pGain.load());
    toneStack.setBass       (pBass.load());
    toneStack.setMid        (pMid.load());
    toneStack.setTreble     (pTreble.load());
    presenceFilter.setPresence(pPresence.load());
    masterVolume.setMaster  (pMaster.load());
    reverb.setMix  (pReverbMix.load());
    reverb.setSize (pReverbSize.load());
    reverb.setDamp (pReverbDamp.load());
    reverb.setWidth(pReverbWidth.load());

    dspReady = true;
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& info)
{
    if (!dspReady) { info.clearActiveBufferRegion(); return; }

    if (paramsChanged.exchange(false))
    {
        noiseGate.setThreshold (pGateThresh.load());
        noiseGate.setAttack    (pGateAttack.load());
        noiseGate.setRelease   (pGateRelease.load());
        noiseGate.setHold      (pGateHold.load());
        powerMetalDist.setDrive(pDrive.load());
        powerMetalDist.setTone (pTone.load());
        powerMetalDist.setTight(pTight.load());
        powerMetalDist.setLevel(pDistLevel.load());
        phaser.setRate         (pPhaserRate.load());
        phaser.setDepth        (pPhaserDepth.load());
        phaser.setFeedback     (pPhaserFeedback.load());
        phaser.setMix          (pPhaserMix.load());
        tapeDelay.setTime      (pDelayTime.load());
        tapeDelay.setFeedback  (pDelayFeedback.load());
        tapeDelay.setMix       (pDelayMix.load());
        tapeDelay.setWow       (pDelayWow.load());
        gainStage.setGain       (pGain.load());
        toneStack.setBass       (pBass.load());
        toneStack.setMid        (pMid.load());
        toneStack.setTreble     (pTreble.load());
        presenceFilter.setPresence(pPresence.load());
        masterVolume.setMaster  (pMaster.load());
        reverb.setMix  (pReverbMix.load());
        reverb.setSize (pReverbSize.load());
        reverb.setDamp (pReverbDamp.load());
        reverb.setWidth(pReverbWidth.load());
    }

    const bool hasStereo = (info.buffer->getNumChannels() >= 2);
    float* outL = info.buffer->getWritePointer(0, info.startSample);
    float* outR = hasStereo ? info.buffer->getWritePointer(1, info.startSample) : nullptr;

    // Pre-fetch backing track block (done once per audio callback, not per sample)
    // backingBuffer was pre-allocated in prepareToPlay — never resize on the audio thread
    const bool btPlaying = transportSource.isPlaying();
    if (btPlaying)
    {
        const int nBT = std::min(info.numSamples, backingBuffer.getNumSamples());
        backingBuffer.clear(0, nBT);
        juce::AudioSourceChannelInfo btInfo(&backingBuffer, 0, nBT);
        transportSource.getNextAudioBlock(btInfo);
    }
    const float btVol      = pBackingVolume.load(std::memory_order_relaxed);
    const bool  tunerOn    = tunerActive.load(std::memory_order_relaxed);
    const int   btChanCnt  = backingBuffer.getNumChannels();
    const int   btSamples  = backingBuffer.getNumSamples();

    for (int i = 0; i < info.numSamples; ++i)
    {
        const float rawIn = outL[i];          // capture before overwrite for tuner
        if (tunerOn)
            pitchDetector.write(rawIn);

        double s = (double)rawIn;

        s = noiseGate.process((float)s);        // gate before distortion
        s = powerMetalDist.process(s);
        s = gainStage.process((float)s);
        s = toneStack.process(s);
        s = presenceFilter.process(s);
        s = phaser.process((float)s);
        s = tapeDelay.process((float)s);

        float reverbL, reverbR;
        reverb.process((float)s, reverbL, reverbR);

        float finalL = masterVolume.process(reverbL);
        float finalR = masterVolume.process(reverbR);

        // Mix backing track into output
        if (btPlaying && i < btSamples)
        {
            finalL += backingBuffer.getSample(0,                   i) * btVol;
            finalR += backingBuffer.getSample(btChanCnt > 1 ? 1 : 0, i) * btVol;
        }

        outL[i] = finalL;
        if (outR != nullptr)
            outR[i] = finalR;
    }
}

void MainComponent::releaseResources()
{
    transportSource.releaseResources();
    dspReady = false;
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(kBackground);

    const int m         = 6;
    const int rowH      = 150;
    const int titleBarH = 28;

    const int row1Y = titleBarH;
    const int row2Y = titleBarH + (rowH + m);
    const int row3Y = titleBarH + (rowH + m) * 2;
    const int row4Y = titleBarH + (rowH + m) * 3;
    const int row5Y = titleBarH + (rowH + m) * 4;

    const int preampEndX = (int)(getWidth() * 0.43f);
    const int powerEndX  = (int)(getWidth() * 0.60f);

    auto drawPanel = [&](juce::Colour fillCol, juce::Colour rimCol,
                         int x, int y, int w, int h, bool grad = false)
    {
        juce::Rectangle<float> r((float)x,(float)y,(float)w,(float)h);
        g.setColour(fillCol);
        g.fillRoundedRectangle(r, 8.0f);
        if (grad)
        {
            juce::ColourGradient gr(rimCol.withAlpha(0.07f), 0,(float)y,
                                    juce::Colours::transparentBlack, 0,(float)(y+50), false);
            g.setGradientFill(gr);
            g.fillRoundedRectangle(r, 8.0f);
        }
        g.setColour(rimCol.darker(0.3f));
        g.drawRoundedRectangle(r, 8.0f, 1.2f);
    };

    // Title bar
    g.setColour(juce::Colour(0xff202024));
    g.fillRect(0, 0, getWidth(), titleBarH);
    g.setColour(kSeparator);
    g.drawHorizontalLine(titleBarH - 1, 0.0f, (float)getWidth());
    g.setColour(kAmpAccent);
    g.setFont(monoFont(13.0f, juce::Font::bold));
    g.drawText("GUITAR AMP MODELER",
               juce::Rectangle<int>(0, 0, getWidth(), titleBarH),
               juce::Justification::centred);

    const int row6Y      = titleBarH + (rowH + m) * 5;
    const int tunerEndX  = (int)(getWidth() * 0.38f);

    drawPanel(kGatePanel,  kGateAccent,  m, row1Y, getWidth()-m*2, rowH, true);
    drawPanel(kDistPanel,  kDistAccent,  m, row2Y, getWidth()-m*2, rowH, true);
    drawPanel(kPhasePanel, kPhaseAccent, m, row3Y, getWidth()-m*2, rowH, true);
    drawPanel(kDelayPanel, kDelayAccent, m, row4Y, getWidth()-m*2, rowH, true);

    drawPanel(kAmpPanel,   kSeparator,              m,            row5Y, preampEndX-m*2,          rowH);
    drawPanel(kPowerPanel, kSeparator,              preampEndX+m, row5Y, powerEndX-preampEndX-m,  rowH);
    drawPanel(kRevPanel,   kRevAccent.darker(0.4f), powerEndX+m,  row5Y, getWidth()-powerEndX-m*2, rowH);

    drawPanel(kTunerPanel, kTunerAccent,            m,            row6Y, tunerEndX-m*2,             rowH, true);
    drawPanel(kBTPanel,    kBTAccent,               tunerEndX+m,  row6Y, getWidth()-tunerEndX-m*2,  rowH, true);
}

void MainComponent::resized()
{
    const int m         = 10;
    const int titleBarH = 28;
    const int rowH      = 150;
    const int labelH    = 18;
    const int valH      = 16;
    const int secH      = 20;
    const int pad       = 4;

    const int row1Y = titleBarH + pad;
    const int row2Y = titleBarH + (rowH + m) + pad;
    const int row3Y = titleBarH + (rowH + m) * 2 + pad;
    const int row4Y = titleBarH + (rowH + m) * 3 + pad;
    const int row5Y = titleBarH + (rowH + m) * 4 + pad;

    const int preampEndX = (int)(getWidth() * 0.43f);
    const int powerEndX  = (int)(getWidth() * 0.60f);
    const int tunerEndX  = (int)(getWidth() * 0.38f);

    audioSettingsButton.setBounds(getWidth() - 190 - m, 3, 190, titleBarH - 6);

    auto placeKnob = [&](juce::Rectangle<int>& area, int w,
                         juce::Slider& knob, juce::Label& nameLbl, juce::Label& valLbl)
    {
        auto col = area.removeFromLeft(w);
        valLbl.setBounds(col.removeFromBottom(valH));
        nameLbl.setBounds(col.removeFromBottom(labelH));
        knob.setBounds(col.reduced(pad, 2));
    };

    auto layout4 = [&](int rowY, juce::Label& secLbl,
                        juce::Slider& k1, juce::Label& n1, juce::Label& v1,
                        juce::Slider& k2, juce::Label& n2, juce::Label& v2,
                        juce::Slider& k3, juce::Label& n3, juce::Label& v3,
                        juce::Slider& k4, juce::Label& n4, juce::Label& v4)
    {
        auto area = juce::Rectangle<int>(m*2, rowY, getWidth()-m*4, rowH-pad*2);
        secLbl.setBounds(area.removeFromTop(secH));
        const int kw = area.getWidth() / 4;
        placeKnob(area, kw, k1, n1, v1);
        placeKnob(area, kw, k2, n2, v2);
        placeKnob(area, kw, k3, n3, v3);
        placeKnob(area, kw, k4, n4, v4);
    };

    layout4(row1Y, gateSectionLabel,
            gateThreshKnob,    gateThreshLabel,    gateThreshValLabel,
            gateAttackKnob,    gateAttackLabel,    gateAttackValLabel,
            gateReleaseKnob,   gateReleaseLabel,   gateReleaseValLabel,
            gateHoldKnob,      gateHoldLabel,      gateHoldValLabel);

    layout4(row2Y, distSectionLabel,
            driveKnob,         driveLabel,         driveValLabel,
            toneKnob,          toneLabel,          toneValLabel,
            tightKnob,         tightLabel,         tightValLabel,
            distLevelKnob,     distLevelLabel,     distLevelValLabel);

    layout4(row3Y, phaserSectionLabel,
            phaserRateKnob,    phaserRateLabel,    phaserRateValLabel,
            phaserDepthKnob,   phaserDepthLabel,   phaserDepthValLabel,
            phaserFeedbackKnob,phaserFeedbackLabel,phaserFeedbackValLabel,
            phaserMixKnob,     phaserMixLabel,     phaserMixValLabel);

    layout4(row4Y, delaySectionLabel,
            delayTimeKnob,     delayTimeLabel,     delayTimeValLabel,
            delayFeedbackKnob, delayFeedbackLabel, delayFeedbackValLabel,
            delayMixKnob,      delayMixLabel,      delayMixValLabel,
            delayWowKnob,      delayWowLabel,      delayWowValLabel);

    const int botH = rowH - pad * 2;

    {
        auto area = juce::Rectangle<int>(m*2, row5Y, preampEndX-m*3, botH);
        preampSectionLabel.setBounds(area.removeFromTop(secH));
        const int kw = area.getWidth() / 4;
        placeKnob(area, kw, gainKnob,   gainLabel,   gainValLabel);
        placeKnob(area, kw, bassKnob,   bassLabel,   bassValLabel);
        placeKnob(area, kw, midKnob,    midLabel,    midValLabel);
        placeKnob(area, kw, trebleKnob, trebleLabel, trebleValLabel);
    }
    {
        auto area = juce::Rectangle<int>(preampEndX+m, row5Y, powerEndX-preampEndX-m, botH);
        powerAmpSectionLabel.setBounds(area.removeFromTop(secH));
        const int kw = area.getWidth() / 2;
        placeKnob(area, kw, presenceKnob, presenceLabel, presenceValLabel);
        placeKnob(area, kw, masterKnob,   masterLabel,   masterValLabel);
    }
    {
        auto area = juce::Rectangle<int>(powerEndX+m, row5Y, getWidth()-powerEndX-m*2, botH);
        reverbSectionLabel.setBounds(area.removeFromTop(secH));
        const int kw = area.getWidth() / 4;
        placeKnob(area, kw, reverbMixKnob,   reverbMixLabel,   reverbMixValLabel);
        placeKnob(area, kw, reverbSizeKnob,  reverbSizeLabel,  reverbSizeValLabel);
        placeKnob(area, kw, reverbDampKnob,  reverbDampLabel,  reverbDampValLabel);
        placeKnob(area, kw, reverbWidthKnob, reverbWidthLabel, reverbWidthValLabel);
    }

    // ---- Row 6: Tuner + Backing Track ---------------------------------------
    const int row6Y = titleBarH + (rowH + m) * 5 + pad;

    // Tuner panel
    {
        auto area = juce::Rectangle<int>(m*2, row6Y, tunerEndX - m*3, rowH - pad*2);
        tunerSectionLabel.setBounds(area.removeFromTop(secH));

        // ON/OFF button
        auto btnRow = area.removeFromTop(30);
        tunerOnButton.setBounds(btnRow.removeFromLeft(120));

        area.removeFromTop(4);

        // Display: big note label on left, cents + freq stacked on right
        auto dispL = area.removeFromLeft(area.getWidth() / 2);
        tunerNoteLabel.setBounds(dispL);   // fills the left half, vertically centred by font

        auto dispR = area;
        tunerCentsLabel.setBounds(dispR.removeFromTop(dispR.getHeight() / 2));
        tunerFreqLabel.setBounds(dispR);
    }

    // Backing Track panel
    {
        auto area = juce::Rectangle<int>(tunerEndX + m, row6Y,
                                         getWidth() - tunerEndX - m*2, rowH - pad*2);
        btSectionLabel.setBounds(area.removeFromTop(secH));

        // Button row
        auto btnRow = area.removeFromTop(30);
        btLoadButton.setBounds(btnRow.removeFromLeft(80));
        btnRow.removeFromLeft(4);
        btPlayButton.setBounds(btnRow.removeFromLeft(80));
        btnRow.removeFromLeft(4);
        btLoopButton.setBounds(btnRow.removeFromLeft(110));

        area.removeFromTop(4);

        // File name at bottom
        btFileLabel.setBounds(area.removeFromBottom(valH));

        // Volume knob takes a fixed slice on the left of the remaining space
        const int kw = 90;
        auto volCol = area.removeFromLeft(kw);
        btVolumeValLabel.setBounds(volCol.removeFromBottom(valH));
        btVolumeLabel.setBounds(volCol.removeFromBottom(labelH));
        btVolumeKnob.setBounds(volCol.reduced(pad, 2));
    }

    juce::ignoreUnused(labelH, valH);
}
