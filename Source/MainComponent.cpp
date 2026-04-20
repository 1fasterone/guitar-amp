#include "MainComponent.h"
#include <BinaryData.h>

//==============================================================================
// ---- Unified hardware metal palette ----------------------------------------
static const juce::Colour kBackground    { 0xff1e1e20 };   // body between panels
static const juce::Colour kSeparator     { 0xff505050 };
// Panel metal shades
static const juce::Colour kMetalTop      { 0xffbebebe };   // gradient highlight
static const juce::Colour kMetalBot      { 0xff939393 };   // gradient shadow
static const juce::Colour kMetalRim      { 0xff484848 };   // border
// Text on silver
static const juce::Colour kLabelText     { 0xff1a1a1a };   // knob name labels
static const juce::Colour kLabelDim      { 0xff3c3c3c };   // section ">> FOO" labels
static const juce::Colour kValueText     { 0xff404040 };   // value readout labels
// Divider line between sub-panels in row 5
static const juce::Colour kDivider       { 0xff585858 };

// Accent colours that remain visible on a silver background
// (used only for tuner note display and BT controls)
static const juce::Colour kTunerAccent   { 0xff006688 };   // dark teal
static const juce::Colour kTunerAccentDim{ 0xff004455 };
static const juce::Colour kBTAccent      { 0xff6600aa };   // dark purple
static const juce::Colour kBTAccentDim   { 0xff440077 };

// Legacy aliases kept so button-colour calls still compile
static const juce::Colour kAmpAccent     { 0xff1a1a1a };   // title-bar text (white below)
static const juce::Colour kGateAccent    = kLabelText;
static const juce::Colour kDistAccent    = kLabelText;
static const juce::Colour kPhaseAccent   = kLabelText;
static const juce::Colour kDelayAccent   = kLabelText;
static const juce::Colour kRevAccent     = kLabelText;

//==============================================================================
// Chrome / brushed-metal knob — single LAF used for every knob
struct MetalKnobLAF : public juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override
    {
        const float r  = (float)juce::jmin(width / 2, height / 2) - 5.0f;
        const float cx = (float)x + width  * 0.5f;
        const float cy = (float)y + height * 0.5f;
        if (r < 5.0f) return;

        // Drop shadow
        g.setColour(juce::Colours::black.withAlpha(0.30f));
        g.fillEllipse(cx - r + 2.0f, cy - r + 3.0f, r * 2.0f, r * 2.0f);

        // Outer bezel ring — dark radial gradient
        {
            juce::ColourGradient bz(juce::Colour(0xff585858), cx - r, cy - r,
                                    juce::Colour(0xff242424), cx + r, cy + r, true);
            g.setGradientFill(bz);
            g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
        }

        const float kr = r - 2.5f;   // inner knob cap radius

        // Main cap — radial gradient: bright upper-left → dark lower-right
        {
            juce::ColourGradient body(
                juce::Colour(0xffdcdcdc), cx - kr * 0.38f, cy - kr * 0.42f,
                juce::Colour(0xff555555), cx + kr * 0.55f, cy + kr * 0.60f, true);
            body.addColour(0.40, juce::Colour(0xffa8a8a8));
            body.addColour(0.70, juce::Colour(0xff787878));
            g.setGradientFill(body);
            g.fillEllipse(cx - kr, cy - kr, kr * 2.0f, kr * 2.0f);
        }

        // Specular hot-spot — bright oval upper-left
        {
            juce::ColourGradient spec(
                juce::Colours::white.withAlpha(0.72f),
                cx - kr * 0.30f, cy - kr * 0.36f,
                juce::Colours::transparentWhite,
                cx + kr * 0.15f, cy + kr * 0.20f, true);
            g.setGradientFill(spec);
            g.fillEllipse(cx - kr, cy - kr, kr * 2.0f, kr * 2.0f);
        }

        // Cap rim
        g.setColour(juce::Colour(0xff242424));
        g.drawEllipse(cx - kr, cy - kr, kr * 2.0f, kr * 2.0f, 1.0f);

        // Indicator dot — inset dark circle
        const float angle   = startAngle + sliderPos * (endAngle - startAngle);
        const float dotDist = kr * 0.66f;
        const float dotX    = cx + std::sin(angle) * dotDist;
        const float dotY    = cy - std::cos(angle) * dotDist;
        const float dotR    = juce::jmax(2.0f, kr * 0.105f);

        g.setColour(juce::Colour(0xff111111));
        g.fillEllipse(dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);
        // tiny inner glint
        g.setColour(juce::Colours::white.withAlpha(0.18f));
        g.fillEllipse(dotX - dotR * 0.45f, dotY - dotR * 0.55f,
                      dotR * 0.8f, dotR * 0.8f);

        // Centre post
        {
            juce::ColourGradient cap(juce::Colour(0xff909090), cx - 2.0f, cy - 2.0f,
                                     juce::Colour(0xff3a3a3a), cx + 2.0f, cy + 2.0f, true);
            g.setGradientFill(cap);
            g.fillEllipse(cx - 3.5f, cy - 3.5f, 7.0f, 7.0f);
        }
    }
};

static MetalKnobLAF gMetalKnobLAF;

//==============================================================================
// VUMeterComponent — Marantz-style blue fluorescent bar display
//==============================================================================
class VUMeterComponent : public juce::Component
{
public:
    VUMeterComponent() = default;

    /** Call from timer callback (message thread). Values in dBFS (-80 to +3). */
    void setLevels (float dBL, float dBR)
    {
        applyBallistics (dBL, displayL, peakHoldL, holdTicksL);
        applyBallistics (dBR, displayR, peakHoldR, holdTicksR);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const int w = getWidth();
        const int h = getHeight();

        // ── Dark display panel background ────────────────────────────────────
        juce::ColourGradient bg (juce::Colour (0xff060810), 0.f, 0.f,
                                 juce::Colour (0xff0a0c18), 0.f, (float)h, false);
        g.setGradientFill (bg);
        g.fillRoundedRectangle (0.f, 0.f, (float)w, (float)h, 5.f);

        // Inner rim — subtle blue tinge
        g.setColour (juce::Colour (0xff1c2540));
        g.drawRoundedRectangle (0.5f, 0.5f, (float)w - 1.f, (float)h - 1.f, 5.f, 1.f);

        // ── Layout ────────────────────────────────────────────────────────────
        const int scaleH = 14;
        const int chanH  = (h - scaleH - 12) / 2;
        const int meterX = 22;
        const int meterW = w - meterX - 6;
        const int chanLY = 4;
        const int chanRY = chanLY + chanH + 4;
        const int scaleY = chanRY + chanH + 4;

        drawChannel (g, meterX, chanLY, meterW, chanH, displayL, peakHoldL, "L");
        drawChannel (g, meterX, chanRY, meterW, chanH, displayR, peakHoldR, "R");
        drawScale   (g, meterX, scaleY, meterW, scaleH);
    }

private:
    //──────────────────────────────────────────────────────────────────────────
    float displayL   = -80.f,  displayR   = -80.f;
    float peakHoldL  = -80.f,  peakHoldR  = -80.f;
    float holdTicksL =   0.f,  holdTicksR =   0.f;

    static constexpr float kMinDB   = -40.f;
    static constexpr float kMaxDB   =   3.f;
    static constexpr int   kNumSegs =  43;     // 1 segment per dB

    //──────────────────────────────────────────────────────────────────────────
    static void applyBallistics (float input,
                                 float& display, float& peak, float& holdTicks)
    {
        input = juce::jlimit (-80.f, kMaxDB, input);

        // VU-style: quick attack, slower release (coefficients at 15 fps)
        if (input > display)
            display += (input - display) * 0.55f;
        else
            display += (input - display) * 0.12f;

        // Peak hold: 2 s @ 15 fps = 30 ticks, then slow fall (~2 dB/s)
        if (input >= peak)        { peak = input; holdTicks = 30.f; }
        else if (holdTicks > 0.f)   holdTicks -= 1.f;
        else                        peak = std::max (peak - 0.13f, input);
    }

    //──────────────────────────────────────────────────────────────────────────
    static int dbToSeg (float db)
    {
        float t = (db - kMinDB) / (kMaxDB - kMinDB);
        return (int) juce::jlimit (0.f, (float)(kNumSegs - 1), t * (float)kNumSegs);
    }

    static float segToDb (int seg)
    {
        return kMinDB + ((float)seg / kNumSegs) * (kMaxDB - kMinDB);
    }

    // Zone-based colour per segment
    static juce::Colour segColour (int seg, bool lit)
    {
        const float db = segToDb (seg);
        if (db >= 0.f)    return lit ? juce::Colour (0xffff2200) : juce::Colour (0xff1a0300);
        if (db >= -3.f)   return lit ? juce::Colour (0xff00d4ff) : juce::Colour (0xff001e28);
        if (db >= -10.f)  return lit ? juce::Colour (0xff009fef) : juce::Colour (0xff000e1e);
        /* low zone */    return lit ? juce::Colour (0xff1166dd) : juce::Colour (0xff000918);
    }

    //──────────────────────────────────────────────────────────────────────────
    void drawChannel (juce::Graphics& g,
                      int x, int y, int w, int h,
                      float dispDB, float peakDB, const char* label)
    {
        const int   litSegs = dbToSeg (dispDB);
        const int   pkSeg   = dbToSeg (peakDB);
        const float segW    = (float)w / kNumSegs;
        const float inner   = (float)h - 2.f;

        // ── Ambient bloom behind lit portion ─────────────────────────────────
        if (litSegs > 0)
        {
            const float litW = litSegs * segW;
            juce::ColourGradient halo (
                juce::Colour (0xff0044bb).withAlpha (0.22f),
                (float)x,               (float)y,
                juce::Colours::transparentBlack,
                (float)x + litW + 18.f, (float)y, false);
            g.setGradientFill (halo);
            g.fillRect ((float)x, (float)(y - 4), litW + 18.f, (float)(h + 8));
        }

        // ── Channel label (L / R) ─────────────────────────────────────────────
        g.setFont   (juce::Font (juce::Font::getDefaultMonospacedFontName(),
                                 10.f, juce::Font::bold));
        g.setColour (juce::Colour (0xff2d6e9a));
        g.drawText  (juce::String (label), x - 20, y, 18, h,
                     juce::Justification::centredRight);

        // ── Dark track ────────────────────────────────────────────────────────
        g.setColour (juce::Colour (0xff030710));
        g.fillRect  ((float)x, (float)(y + 1), (float)w, inner);

        // ── Segments ─────────────────────────────────────────────────────────
        for (int i = 0; i < kNumSegs; ++i)
        {
            const bool  lit = (i < litSegs);
            const float sx  = (float)x + i * segW + 0.5f;
            const float sw  = segW - 1.5f;

            g.setColour (segColour (i, lit));
            g.fillRect  (sx, (float)(y + 1), sw, inner);

            // Per-segment bloom on lit segments
            if (lit)
            {
                g.setColour (segColour (i, true).withAlpha (0.28f));
                g.fillRect  (sx - 1.f, (float)(y - 1), sw + 2.f, (float)(h + 2));
            }
        }

        // ── Peak hold tick ────────────────────────────────────────────────────
        if (peakDB > kMinDB && pkSeg > 0 && pkSeg < kNumSegs)
        {
            const float  px  = (float)x + pkSeg * segW + 0.5f;
            const float  pw  = segW - 1.5f;
            juce::Colour pkC = (peakDB >= 0.f) ? juce::Colour (0xffff4400)
                                                : juce::Colour (0xffd0ecff);
            g.setColour (pkC);
            g.fillRect  (px, (float)(y + 1), pw, inner);
            // Glow halo around tick
            g.setColour (pkC.withAlpha (0.50f));
            g.fillRect  (px - 1.5f, (float)(y - 1), pw + 3.f, (float)(h + 2));
        }
    }

    //──────────────────────────────────────────────────────────────────────────
    void drawScale (juce::Graphics& g, int x, int y, int w, int scaleH)
    {
        const float segW = (float)w / kNumSegs;
        g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(),
                               9.f, juce::Font::plain));

        struct ScaleTick { float db; const char* txt; };
        const ScaleTick ticks[] = {
            {-40.f,"-40"}, {-30.f,"-30"}, {-20.f,"-20"},
            {-10.f,"-10"}, { -6.f, "-6"}, { -3.f, "-3"},
            {  0.f,  "0"}, {  3.f, "+3"}
        };

        for (const auto& t : ticks)
        {
            const float tx = (float)x + dbToSeg (t.db) * segW;
            g.setColour (juce::Colour (0xff152840));
            g.fillRect  (tx, (float)y, 1.f, 4.f);
            g.setColour (juce::Colour (0xff2255aa));
            g.drawText  (juce::String (t.txt),
                         (int)tx - 12, y + 4, 24, scaleH - 4,
                         juce::Justification::centred);
        }
    }
};

//==============================================================================
static juce::Font monoFont(float size, int style = juce::Font::plain)
{
    return juce::Font(juce::Font::getDefaultMonospacedFontName(), size, style);
}

//==============================================================================
MainComponent::MainComponent()
{
    formatManager.registerBasicFormats();

    auto makeSectionLabel = [&](juce::Label& lbl, const juce::String& text, juce::Colour /*col*/)
    {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setFont(monoFont(11.0f, juce::Font::bold));
        lbl.setColour(juce::Label::textColourId, kLabelDim);
        lbl.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(lbl);
    };

    // ---- Noise Gate ---------------------------------------------------------
    setupKnob(gateThreshKnob,  gateThreshLabel,  gateThreshValLabel,  "THRESHOLD", 0.30, kGateAccent);
    setupKnob(gateAttackKnob,  gateAttackLabel,  gateAttackValLabel,  "ATTACK",    0.20, kGateAccent);
    setupKnob(gateReleaseKnob, gateReleaseLabel, gateReleaseValLabel, "RELEASE",   0.50, kGateAccent);
    setupKnob(gateHoldKnob,    gateHoldLabel,    gateHoldValLabel,    "HOLD",      0.30, kGateAccent);
    makeSectionLabel(gateSectionLabel, ">>  NOISE GATE", kLabelDim);

    // ---- Distortion ---------------------------------------------------------
    setupKnob(driveKnob,     driveLabel,     driveValLabel,     "DRIVE", 0.50, kDistAccent);
    setupKnob(toneKnob,      toneLabel,      toneValLabel,      "TONE",  0.50, kDistAccent);
    setupKnob(tightKnob,     tightLabel,     tightValLabel,     "TIGHT", 0.30, kDistAccent);
    setupKnob(distLevelKnob, distLevelLabel, distLevelValLabel, "LEVEL", 0.70, kDistAccent);
    makeSectionLabel(distSectionLabel, ">>  POWER METAL DISTORTION", kLabelDim);

    // ---- Phaser -------------------------------------------------------------
    setupKnob(phaserRateKnob,     phaserRateLabel,     phaserRateValLabel,     "RATE",     0.30, kPhaseAccent);
    setupKnob(phaserDepthKnob,    phaserDepthLabel,    phaserDepthValLabel,    "DEPTH",    0.70, kPhaseAccent);
    setupKnob(phaserFeedbackKnob, phaserFeedbackLabel, phaserFeedbackValLabel, "FEEDBACK", 0.40, kPhaseAccent);
    setupKnob(phaserMixKnob,      phaserMixLabel,      phaserMixValLabel,      "MIX",      0.50, kPhaseAccent);
    makeSectionLabel(phaserSectionLabel, ">>  PHASER", kLabelDim);

    // ---- Tape Delay ---------------------------------------------------------
    setupKnob(delayTimeKnob,     delayTimeLabel,     delayTimeValLabel,     "TIME",     0.35, kDelayAccent);
    setupKnob(delayFeedbackKnob, delayFeedbackLabel, delayFeedbackValLabel, "FEEDBACK", 0.40, kDelayAccent);
    setupKnob(delayMixKnob,      delayMixLabel,      delayMixValLabel,      "MIX",      0.30, kDelayAccent);
    setupKnob(delayWowKnob,      delayWowLabel,      delayWowValLabel,      "WOW",      0.25, kDelayAccent);
    makeSectionLabel(delaySectionLabel, ">>  TAPE ECHO", kLabelDim);

    // ---- Amp preamp ---------------------------------------------------------
    setupKnob(gainKnob,   gainLabel,   gainValLabel,   "GAIN",   0.50, kAmpAccent);
    setupKnob(bassKnob,   bassLabel,   bassValLabel,   "BASS",   0.50, kAmpAccent);
    setupKnob(midKnob,    midLabel,    midValLabel,    "MID",    0.50, kAmpAccent);
    setupKnob(trebleKnob, trebleLabel, trebleValLabel, "TREBLE", 0.50, kAmpAccent);
    makeSectionLabel(preampSectionLabel, ">>  PREAMP", kLabelDim);

    // ---- Amp power ----------------------------------------------------------
    setupKnob(presenceKnob, presenceLabel, presenceValLabel, "PRESENCE", 0.50, kAmpAccent);
    setupKnob(masterKnob,   masterLabel,   masterValLabel,   "MASTER",   0.80, kAmpAccent);
    makeSectionLabel(powerAmpSectionLabel, ">>  POWER AMP", kLabelDim);

    // ---- Reverb -------------------------------------------------------------
    setupKnob(reverbMixKnob,   reverbMixLabel,   reverbMixValLabel,   "MIX",   0.25, kRevAccent);
    setupKnob(reverbSizeKnob,  reverbSizeLabel,  reverbSizeValLabel,  "SIZE",  0.50, kRevAccent);
    setupKnob(reverbDampKnob,  reverbDampLabel,  reverbDampValLabel,  "DAMP",  0.50, kRevAccent);
    setupKnob(reverbWidthKnob, reverbWidthLabel, reverbWidthValLabel, "WIDTH", 1.00, kRevAccent);
    makeSectionLabel(reverbSectionLabel, ">>  REVERB", kLabelDim);

    // ---- Cab Sim ------------------------------------------------------------
    makeSectionLabel(cabSectionLabel, ">>  CAB SIM", kLabelDim);

    cabOnButton.setClickingTogglesState(true);
    cabOnButton.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff909090));
    cabOnButton.setColour(juce::TextButton::textColourOffId,  kLabelText);
    cabOnButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff404040));
    cabOnButton.setColour(juce::TextButton::textColourOnId,   juce::Colours::white);
    cabOnButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    cabOnButton.onClick = [this]
    {
        cabActive = cabOnButton.getToggleState();
        cabOnButton.setButtonText(cabActive.load() ? "CAB: ON" : "CAB: OFF");
    };
    addAndMakeVisible(cabOnButton);

    cabLoadButton.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff909090));
    cabLoadButton.setColour(juce::TextButton::textColourOffId, kLabelText);
    cabLoadButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    cabLoadButton.onClick = [this]
    {
        cabFileChooser = std::make_unique<juce::FileChooser>(
            "Load Cabinet IR",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.wav;*.aiff");
        cabFileChooser->launchAsync(
            juce::FileBrowserComponent::openMode |
            juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                auto result = fc.getResult();
                if (!result.existsAsFile()) return;
                cabSim.loadIR(result);
                juce::MessageManager::callAsync([this, name = result.getFileName()]
                {
                    cabFileLabel.setText(name, juce::dontSendNotification);
                    if (!cabOnButton.getToggleState())
                    {
                        cabOnButton.setToggleState(true, juce::dontSendNotification);
                        cabActive = true;
                        cabOnButton.setButtonText("CAB: ON");
                    }
                });
            });
    };
    addAndMakeVisible(cabLoadButton);

    cabBlendKnob.setLookAndFeel(&gMetalKnobLAF);
    cabBlendKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    cabBlendKnob.setRange(0.0, 1.0);
    cabBlendKnob.setValue(1.0, juce::dontSendNotification);
    cabBlendKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    cabBlendKnob.onValueChange = [this] { pCabBlend = (float)cabBlendKnob.getValue(); };
    addAndMakeVisible(cabBlendKnob);

    cabBlendLabel.setText("BLEND", juce::dontSendNotification);
    cabBlendLabel.setFont(monoFont(12.0f, juce::Font::bold));
    cabBlendLabel.setColour(juce::Label::textColourId, kLabelText);
    cabBlendLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(cabBlendLabel);

    cabBlendValLabel.setText("100%", juce::dontSendNotification);
    cabBlendValLabel.setFont(monoFont(11.0f));
    cabBlendValLabel.setColour(juce::Label::textColourId, kValueText);
    cabBlendValLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(cabBlendValLabel);

    cabFileLabel.setText("No IR loaded", juce::dontSendNotification);
    cabFileLabel.setFont(monoFont(10.0f));
    cabFileLabel.setColour(juce::Label::textColourId, kValueText);
    cabFileLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(cabFileLabel);

    // ---- Tuner --------------------------------------------------------------
    makeSectionLabel(tunerSectionLabel, ">>  CHROMATIC TUNER", kTunerAccentDim);

    tunerOnButton.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff909090));
    tunerOnButton.setColour(juce::TextButton::textColourOffId,  kLabelText);
    tunerOnButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff404040));
    tunerOnButton.setColour(juce::TextButton::textColourOnId,   juce::Colours::white);
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
    styleLabel(tunerNoteLabel,  40.0f, juce::Font::bold,  kTunerAccent);   // stays teal
    styleLabel(tunerCentsLabel, 14.0f, juce::Font::plain, kValueText);
    styleLabel(tunerFreqLabel,  11.0f, juce::Font::plain, kValueText);    // dark on silver

    // ---- Backing Track ------------------------------------------------------
    makeSectionLabel(btSectionLabel, ">>  BACKING TRACK", kBTAccentDim);

    auto styleBtn = [&](juce::TextButton& btn, juce::Colour bg, juce::Colour fg)
    {
        btn.setColour(juce::TextButton::buttonColourId,  bg);
        btn.setColour(juce::TextButton::textColourOffId, fg);
        btn.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        addAndMakeVisible(btn);
    };

    styleBtn(btLoadButton, juce::Colour(0xff909090), kLabelText);
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

    styleBtn(btPlayButton, juce::Colour(0xff909090), kLabelText);
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
    btLoopButton.setColour(juce::TextButton::buttonColourId,    juce::Colour(0xff909090));
    btLoopButton.setColour(juce::TextButton::textColourOffId,   kLabelText);
    btLoopButton.setColour(juce::TextButton::buttonOnColourId,  juce::Colour(0xff404040));
    btLoopButton.setColour(juce::TextButton::textColourOnId,    juce::Colours::white);
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
    btVolumeKnob.setLookAndFeel(&gMetalKnobLAF);
    btVolumeKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    btVolumeKnob.setRange(0.0, 1.0);
    btVolumeKnob.setValue(0.80, juce::dontSendNotification);
    btVolumeKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    btVolumeKnob.onValueChange = [this] { pBackingVolume = (float)btVolumeKnob.getValue(); };
    addAndMakeVisible(btVolumeKnob);

    btVolumeLabel.setText("VOLUME", juce::dontSendNotification);
    styleLabel(btVolumeLabel,  12.0f, juce::Font::bold,  kLabelText);
    btVolumeValLabel.setText("80%", juce::dontSendNotification);
    styleLabel(btVolumeValLabel, 11.0f, juce::Font::plain, kValueText);

    btFileLabel.setText("No file loaded", juce::dontSendNotification);
    styleLabel(btFileLabel, 11.0f, juce::Font::plain, kValueText,
               juce::Justification::centredLeft);

    // ---- Audio settings button ----------------------------------------------
    audioSettingsButton.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff383838));
    audioSettingsButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
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

    // ---- Bypass buttons (one per effect section) ----------------------------
    auto setupBypass = [&](juce::TextButton& btn, std::atomic<bool>& flag)
    {
        btn.setClickingTogglesState (true);
        // Active state (toggle=false): dark panel, Marantz-blue "ON" text
        btn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff252525));
        btn.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff1a7acc));
        // Bypassed state (toggle=true): darker, dim grey "BYP" text
        btn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff141414));
        btn.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff555555));
        btn.setMouseCursor (juce::MouseCursor::PointingHandCursor);
        btn.onClick = [&btn, &flag]
        {
            flag = btn.getToggleState();
            btn.setButtonText (btn.getToggleState() ? "BYP" : "ON");
        };
        addAndMakeVisible (btn);
    };

    setupBypass (gateBypassBtn,   gateBypass);
    setupBypass (distBypassBtn,   distBypass);
    setupBypass (phaserBypassBtn, phaserBypass);
    setupBypass (delayBypassBtn,  delayBypass);
    setupBypass (preampBypassBtn, preampBypass);
    setupBypass (reverbBypassBtn, reverbBypass);

    // ---- VU Meter -----------------------------------------------------------
    vuMeter = std::make_unique<VUMeterComponent>();
    addAndMakeVisible(*vuMeter);

    startTimer(66);   // 15 fps value-label refresh
    setSize(1000, 940);
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
    juce::ignoreUnused(accentColour);   // all knobs now use the single chrome LAF
    knob.setLookAndFeel(&gMetalKnobLAF);
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
    nameLabel.setFont(monoFont(12.0f, juce::Font::bold));
    nameLabel.setColour(juce::Label::textColourId, kLabelText);
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

    // ---- Cab sim blend readout ----------------------------------------------
    cabBlendValLabel.setText(
        juce::String((int)(cabBlendKnob.getValue() * 100.0)) + "%",
        juce::dontSendNotification);

    // ---- Backing track button state -----------------------------------------
    btPlayButton.setButtonText(transportSource.isPlaying() ? "STOP" : "PLAY");
    btVolumeValLabel.setText(
        juce::String((int)(btVolumeKnob.getValue() * 100.0)) + "%",
        juce::dontSendNotification);

    // ---- VU meter -----------------------------------------------------------
    if (vuMeter != nullptr && dspReady.load (std::memory_order_relaxed))
    {
        auto toDb = [](float lin) -> float
        {
            return lin > 1.0e-7f ? 20.0f * std::log10 (lin) : -80.0f;
        };
        vuMeter->setLevels (toDb (vuPeakL.load (std::memory_order_relaxed)),
                            toDb (vuPeakR.load (std::memory_order_relaxed)));
    }
}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    pitchDetector.setSampleRate(sampleRate);
    cabSim.prepare(sampleRate, samplesPerBlockExpected);
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
    // Cache bypass flags once per block (avoid per-sample atomic reads)
    const bool bpGate   = gateBypass  .load (std::memory_order_relaxed);
    const bool bpDist   = distBypass  .load (std::memory_order_relaxed);
    const bool bpPreamp = preampBypass.load (std::memory_order_relaxed);
    const bool bpPhaser = phaserBypass.load (std::memory_order_relaxed);
    const bool bpDelay  = delayBypass .load (std::memory_order_relaxed);
    const bool bpReverb = reverbBypass.load (std::memory_order_relaxed);

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

    // ---- Loop 1: mono per-sample chain (gate → … → tape delay) → outL ------
    for (int i = 0; i < info.numSamples; ++i)
    {
        const float rawIn = outL[i];
        if (tunerOn)
            pitchDetector.write(rawIn);

        double s = (double)rawIn;
        if (!bpGate)   s = noiseGate.process ((float)s);
        if (!bpDist)   s = powerMetalDist.process (s);
        if (!bpPreamp) { s = gainStage.process ((float)s); s = toneStack.process (s); }
        s = presenceFilter.process (s);   // presence always active
        if (!bpPhaser) s = phaser.process ((float)s);
        if (!bpDelay)  s = tapeDelay.process ((float)s);
        outL[i] = (float)s;
    }

    // ---- Cab sim block (mono convolution, in-place on outL) -----------------
    if (cabActive.load(std::memory_order_relaxed))
        cabSim.processBlock(outL, info.numSamples,
                            pCabBlend.load(std::memory_order_relaxed));

    // ---- Loop 2: reverb + master + backing track mix → final stereo ---------
    for (int i = 0; i < info.numSamples; ++i)
    {
        float reverbL, reverbR;
        if (!bpReverb)
            reverb.process (outL[i], reverbL, reverbR);
        else
            reverbL = reverbR = outL[i];

        float finalL = masterVolume.process(reverbL);
        float finalR = masterVolume.process(reverbR);

        if (btPlaying && i < btSamples)
        {
            finalL += backingBuffer.getSample(0,                     i) * btVol;
            finalR += backingBuffer.getSample(btChanCnt > 1 ? 1 : 0, i) * btVol;
        }

        outL[i] = finalL;
        if (outR != nullptr)
            outR[i] = finalR;
    }

    // ---- Peak capture for VU meters (audio thread) --------------------------
    // Compute block peak, then max-with-slow-decay into the shared atomics so
    // the timer always sees the highest recent sample without snapping to zero.
    {
        float blkL = 0.0f, blkR = 0.0f;
        for (int i = 0; i < info.numSamples; ++i)
        {
            blkL = std::max (blkL, std::abs (outL[i]));
            if (outR) blkR = std::max (blkR, std::abs (outR[i]));
        }
        if (outR == nullptr) blkR = blkL;

        // 0.9985 per block ≈ -4.5 dB/s decay when silent
        const float decay = 0.9985f;
        vuPeakL.store (std::max (blkL, vuPeakL.load (std::memory_order_relaxed) * decay),
                       std::memory_order_relaxed);
        vuPeakR.store (std::max (blkR, vuPeakR.load (std::memory_order_relaxed) * decay),
                       std::memory_order_relaxed);
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
    const int rowH      = 140;
    const int titleBarH = 68;

    const int row1Y = titleBarH;
    const int row2Y = titleBarH + (rowH + m);
    const int row3Y = titleBarH + (rowH + m) * 2;
    const int row4Y = titleBarH + (rowH + m) * 3;
    const int row5Y = titleBarH + (rowH + m) * 4;

    const int preampEndX = (int)(getWidth() * 0.39f);
    const int powerEndX  = (int)(getWidth() * 0.53f);
    const int cabEndX    = (int)(getWidth() * 0.73f);

    // ---- Title bar — dark panel houses the BLUESTEEL brand + VU meters ------
    {
        // Very dark, almost-black background — frames the display like a Marantz front panel
        juce::ColourGradient tb (juce::Colour (0xff0e1018), 0, 0,
                                 juce::Colour (0xff080a12), 0, (float)titleBarH, false);
        g.setGradientFill (tb);
        g.fillRect (0, 0, getWidth(), titleBarH);

        // Top edge — faint blue highlight
        g.setColour (juce::Colour (0xff1c2a40).withAlpha (0.6f));
        g.drawHorizontalLine (0, 0.0f, (float)getWidth());

        // Bottom separator — subtle blue rule
        g.setColour (juce::Colour (0xff1c2a40));
        g.drawHorizontalLine (titleBarH - 1, 0.0f, (float)getWidth());

        // ── BlueSteel logo icon — left side ──────────────────────────────────
        {
            auto logo = juce::ImageCache::getFromMemory (
                BinaryData::bluesteel_png, BinaryData::bluesteel_pngSize);
            if (logo.isValid())
            {
                const int logoH = titleBarH - 6;
                const int logoW = juce::roundToInt (
                    logo.getWidth() * (float)logoH / logo.getHeight());
                g.drawImageWithin (logo,
                    3, 3, logoW, logoH,
                    juce::RectanglePlacement::centred |
                    juce::RectanglePlacement::onlyReduceInSize);
            }
        }
    }

    const int row6Y     = titleBarH + (rowH + m) * 5;
    const int tunerEndX = (int)(getWidth() * 0.38f);

    // ---- Brushed-metal panel painter ----------------------------------------
    auto drawMetalPanel = [&](int x, int y, int w, int h)
    {
        juce::Rectangle<float> r((float)x, (float)y, (float)w, (float)h);

        // Base vertical gradient
        juce::ColourGradient base(kMetalTop, 0.0f, (float)y,
                                  kMetalBot, 0.0f, (float)(y + h), false);
        g.setGradientFill(base);
        g.fillRoundedRectangle(r, 8.0f);

        // Horizontal brush-marks
        {
            juce::Graphics::ScopedSaveState ss(g);
            g.reduceClipRegion(r.toNearestInt());
            for (int ly = y; ly < y + h; ++ly)
            {
                float alpha = (ly % 4 == 0) ? 0.055f
                            : (ly % 2 == 0) ? 0.020f
                                            : 0.008f;
                g.setColour(juce::Colours::white.withAlpha(alpha));
                g.drawHorizontalLine(ly, (float)x, (float)(x + w));
            }
        }

        // Top-edge specular highlight
        g.setColour(juce::Colours::white.withAlpha(0.60f));
        g.drawLine((float)(x + 9), (float)(y + 1),
                   (float)(x + w - 9), (float)(y + 1), 1.0f);

        // Bottom-edge shadow
        g.setColour(juce::Colours::black.withAlpha(0.22f));
        g.drawLine((float)(x + 9), (float)(y + h - 1),
                   (float)(x + w - 9), (float)(y + h - 1), 1.0f);

        // Panel border
        g.setColour(kMetalRim);
        g.drawRoundedRectangle(r, 8.0f, 1.5f);
    };

    // Rows 1-4
    drawMetalPanel(m, row1Y, getWidth() - m * 2, rowH);
    drawMetalPanel(m, row2Y, getWidth() - m * 2, rowH);
    drawMetalPanel(m, row3Y, getWidth() - m * 2, rowH);
    drawMetalPanel(m, row4Y, getWidth() - m * 2, rowH);

    // Row 5 — four sub-panels: preamp | power amp | cab sim | reverb
    drawMetalPanel(m,              row5Y, preampEndX - m * 2,          rowH);
    drawMetalPanel(preampEndX + m, row5Y, powerEndX - preampEndX - m,  rowH);
    drawMetalPanel(powerEndX + m,  row5Y, cabEndX - powerEndX - m,     rowH);
    drawMetalPanel(cabEndX + m,    row5Y, getWidth() - cabEndX - m * 2, rowH);

    // Row 6 — tuner + backing track
    drawMetalPanel(m,              row6Y, tunerEndX - m * 2,              rowH);
    drawMetalPanel(tunerEndX + m,  row6Y, getWidth() - tunerEndX - m * 2, rowH);
}

void MainComponent::resized()
{
    const int m         = 10;
    const int titleBarH = 68;
    const int rowH      = 140;
    const int labelH    = 18;
    const int valH      = 16;
    const int secH      = 20;
    const int pad       = 4;

    const int row1Y = titleBarH + pad;
    const int row2Y = titleBarH + (rowH + m) + pad;
    const int row3Y = titleBarH + (rowH + m) * 2 + pad;
    const int row4Y = titleBarH + (rowH + m) * 3 + pad;
    const int row5Y = titleBarH + (rowH + m) * 4 + pad;

    const int preampEndX = (int)(getWidth() * 0.39f);
    const int powerEndX  = (int)(getWidth() * 0.53f);
    const int cabEndX    = (int)(getWidth() * 0.73f);
    const int tunerEndX  = (int)(getWidth() * 0.38f);

    // Settings button — right side of title bar, vertically centred
    const int btnH = 26;
    audioSettingsButton.setBounds (getWidth() - 200 - m, (titleBarH - btnH) / 2, 200, btnH);

    // VU meter — occupies the centre of the title bar between branding and button
    if (vuMeter != nullptr)
    {
        const int vuX = 70;   // just right of the logo icon
        const int vuW = (getWidth() - 200 - m - 8) - vuX;
        vuMeter->setBounds (vuX, 4, vuW, titleBarH - 8);
    }

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

    // Bypass buttons — positioned in top-right of each section header
    {
        const int bw = 46, bh = secH - 2;
        const int bx = getWidth() - m * 2 - bw;
        gateBypassBtn  .setBounds (bx, row1Y + 1, bw, bh);
        distBypassBtn  .setBounds (bx, row2Y + 1, bw, bh);
        phaserBypassBtn.setBounds (bx, row3Y + 1, bw, bh);
        delayBypassBtn .setBounds (bx, row4Y + 1, bw, bh);
        // Preamp — right side of preamp sub-panel
        preampBypassBtn.setBounds (preampEndX - m - bw, row5Y + 1, bw, bh);
        // Reverb — right side of reverb sub-panel
        reverbBypassBtn.setBounds (getWidth() - m * 2 - bw, row5Y + 1, bw, bh);
    }

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

    // Preamp
    {
        auto area = juce::Rectangle<int>(m*2, row5Y, preampEndX - m*3, botH);
        preampSectionLabel.setBounds(area.removeFromTop(secH));
        const int kw = area.getWidth() / 4;
        placeKnob(area, kw, gainKnob,   gainLabel,   gainValLabel);
        placeKnob(area, kw, bassKnob,   bassLabel,   bassValLabel);
        placeKnob(area, kw, midKnob,    midLabel,    midValLabel);
        placeKnob(area, kw, trebleKnob, trebleLabel, trebleValLabel);
    }
    // Power Amp
    {
        auto area = juce::Rectangle<int>(preampEndX + m, row5Y,
                                         powerEndX - preampEndX - m, botH);
        powerAmpSectionLabel.setBounds(area.removeFromTop(secH));
        const int kw = area.getWidth() / 2;
        placeKnob(area, kw, presenceKnob, presenceLabel, presenceValLabel);
        placeKnob(area, kw, masterKnob,   masterLabel,   masterValLabel);
    }
    // Cab Sim
    {
        auto area = juce::Rectangle<int>(powerEndX + m, row5Y,
                                         cabEndX - powerEndX - m, botH);
        cabSectionLabel.setBounds(area.removeFromTop(secH));

        // ON and LOAD buttons stacked on the left
        auto btnCol = area.removeFromLeft(area.getWidth() - 88);
        cabOnButton.setBounds  (btnCol.removeFromTop(28));
        btnCol.removeFromTop(4);
        cabLoadButton.setBounds(btnCol.removeFromTop(28));

        // File label at bottom
        cabFileLabel.setBounds(area.removeFromBottom(valH));

        // Blend knob fills remaining right column
        area.removeFromTop(2);
        cabBlendValLabel.setBounds(area.removeFromBottom(valH));
        cabBlendLabel.setBounds   (area.removeFromBottom(labelH));
        cabBlendKnob.setBounds    (area.reduced(pad, 2));
    }
    // Reverb
    {
        auto area = juce::Rectangle<int>(cabEndX + m, row5Y,
                                         getWidth() - cabEndX - m*2, botH);
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
