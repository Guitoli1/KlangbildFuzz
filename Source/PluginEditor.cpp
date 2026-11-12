#include "PluginEditor.h"

using namespace vintage;

namespace { juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); } }

//==============================================================================
FuzzAudioProcessorEditor::FuzzAudioProcessorEditor (FuzzAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    fuzzKnob   = &addKnob ("fuzz", "FUZZ", true);
    volKnob    = &addKnob ("volume", u8 ("LAUTST\xc3\x84RKE"), true);
    typeKnob   = &addKnob ("type", "TYP", false);
    biasKnob   = &addKnob ("bias", "BIAS", false);
    toneKnob   = &addKnob ("tone", "TON", false);
    guitarKnob = &addKnob ("guitar", "GITARREN-VOL.", false);
    bTypeKnob  = &addKnob ("boostType", "BOOSTER", false);
    boostKnob  = &addKnob ("boost", "BOOST", false);
    fuzzKnob->slider.getProperties().set ("stepLabels", "0|1|2|3|4|5|6|7|8|9|10");
    volKnob->slider.getProperties().set ("stepLabels", "-24|-18|-12|-6|0|+6|+12");
    typeKnob->slider.getProperties().set ("stepLabels", "GE|SI|BEND.|MUFF");
    biasKnob->slider.getProperties().set ("stepLabels", "SPUCKT||||SATT");
    toneKnob->slider.getProperties().set ("stepLabels", "DUNKEL||||HELL");
    guitarKnob->slider.getProperties().set ("stepLabels", "0|2|4|6|8|10");
    bTypeKnob->slider.getProperties().set ("stepLabels", u8 ("CLEAN|H\xc3\x96HEN|MITTEN"));
    boostKnob->slider.getProperties().set ("stepLabels", "0|6|12|18|24 DB");
    typeKnob->slider.setMouseDragSensitivity (130);
    bTypeKnob->slider.setMouseDragSensitivity (110);

    setupSwitch (fuzzSwitch, "fuzzOn", "AN", "AUS", lampRed);
    setupSwitch (boostSwitch, "boostOn", "AN", "AUS", lampAmber);
    setupSwitch (orderSwitch, "boostFirst", u8 ("B \xe2\x86\x92 F"), u8 ("F \xe2\x86\x92 B"), lampGreen);
    setupSwitch (octSwitch, "octave", "AN", "AUS", lampAmber);

    presetBox.getProperties().set ("plate", true);
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    curveWin.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (curveWin);
    typeLabel.setJustificationType (juce::Justification::centred);
    typeLabel.setFont (lnf.labelFont (15.0f, 0.15f));
    typeLabel.setColour (juce::Label::textColourId, ink);
    typeLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (typeLabel);

    setSize (1240, 660);
    startTimerHz (60);
}

FuzzAudioProcessorEditor::~FuzzAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

FuzzAudioProcessorEditor::Knob& FuzzAudioProcessorEditor::addKnob (const juce::String& paramID, const juce::String& name, bool big)
{
    auto k = std::make_unique<Knob>();
    k->name = name;
    k->big = big;
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    k->slider.setMouseDragSensitivity (big ? 320 : 220);
    addAndMakeVisible (k->slider);
    k->attachment = std::make_unique<SliderAttachment> (proc.apvts, paramID, k->slider);
    if (auto* rp = proc.apvts.getParameter (paramID))
        k->slider.setDoubleClickReturnValue (true, rp->convertFrom0to1 (rp->getDefaultValue()));
    knobs.push_back (std::move (k));
    return *knobs.back();
}

void FuzzAudioProcessorEditor::setupSwitch (juce::ToggleButton& b, const juce::String& paramID,
                                            const juce::String& onText, const juce::String& offText, juce::Colour lampColour)
{
    b.setButtonText ("x");
    b.getProperties().set ("onText", onText);
    b.getProperties().set ("offText", offText);
    b.getProperties().set ("lamp", (int) lampColour.getARGB());
    addAndMakeVisible (b);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, paramID, b));
}

//==============================================================================
void FuzzAudioProcessorEditor::renderTextures()
{
    tolexImage = juce::Image (juce::Image::RGB, juce::jmax (1, getWidth()), juce::jmax (1, getHeight()), false);
    {
        juce::Image::BitmapData data (tolexImage, juce::Image::BitmapData::writeOnly);
        juce::Random rnd (11);
        for (int y = 0; y < tolexImage.getHeight(); ++y)
            for (int x = 0; x < tolexImage.getWidth(); ++x)
            {
                const int weave = ((x + y) % 4 == 0 || (x - y + 4096) % 4 == 0) ? -5 : 0;
                const int n = rnd.nextInt (13) - 6 + weave;
                data.setPixelColour (x, y, juce::Colour ((juce::uint8) juce::jlimit (0, 255, 34 + n),
                                                         (juce::uint8) juce::jlimit (0, 255, 28 + n),
                                                         (juce::uint8) juce::jlimit (0, 255, 24 + n)));
            }
    }
    if (faceplate.isEmpty())
        return;
    faceplateImage = juce::Image (juce::Image::ARGB, faceplate.getWidth(), faceplate.getHeight(), true);
    juce::Graphics g (faceplateImage);
    const auto r = faceplateImage.getBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (cream.brighter (0.06f), 0, 0, creamDark, 0, r.getHeight(), false));
    g.fillRoundedRectangle (r, 6.0f);
    juce::Random rnd (5);
    for (int y = 0; y < faceplate.getHeight(); ++y)
    {
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colour (0xff6b5534)).withAlpha (0.015f + rnd.nextFloat() * 0.035f));
        g.drawHorizontalLine (y, 0.0f, r.getWidth());
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(),
                                             juce::Colour (0x30694a1c), 0, 0, true));
    g.fillRoundedRectangle (r, 6.0f);
}

void FuzzAudioProcessorEditor::drawEngraved (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r,
                                             juce::Font f, juce::Justification j)
{
    g.setFont (f);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawFittedText (text, r.translated (0.0f, 1.0f).toNearestInt(), j, 2, 1.0f);
    g.setColour (ink);
    g.drawFittedText (text, r.toNearestInt(), j, 2, 1.0f);
}

void FuzzAudioProcessorEditor::timerCallback()
{
    auto& a = proc.apvts;
    const int type = (int) a.getRawParameterValue ("type")->load();
    const float fz = a.getRawParameterValue ("fuzz")->load() * 0.01f, bias = a.getRawParameterValue ("bias")->load() * 0.01f;
    const float lev = proc.inputPeak.load();
    const bool on = a.getRawParameterValue ("fuzzOn")->load() > 0.5f;
    if (type != curveWin.type || ! juce::exactlyEqual (fz, curveWin.fuzz) || ! juce::exactlyEqual (bias, curveWin.bias)
        || std::abs (lev - curveWin.level) > 0.002f || on != curveWin.on)
    {
        curveWin.type = type; curveWin.fuzz = fz; curveWin.bias = bias; curveWin.level = lev; curveWin.on = on;
        curveWin.repaint();
    }
    const bool boostOn = a.getRawParameterValue ("boostOn")->load() > 0.5f;
    for (auto* k : { fuzzKnob, typeKnob, biasKnob, toneKnob }) k->slider.setAlpha (on ? 1.0f : 0.45f);
    for (auto* k : { bTypeKnob, boostKnob }) k->slider.setAlpha (boostOn ? 1.0f : 0.45f);
    orderSwitch.setAlpha (on && boostOn ? 1.0f : 0.45f);
    octSwitch.setAlpha (on ? 1.0f : 0.45f);

    const char* names[] = { "GERMANIUM", "SILIZIUM", "TONE BENDER", "MUFF" };
    const auto txt = on ? juce::String (names[juce::jlimit (0, 3, type)]) : juce::String ("FUZZ AUS");
    if (typeLabel.getText() != txt)
        typeLabel.setText (txt, juce::dontSendNotification);

    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}

void FuzzAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (tolexImage, 0, 0);

    const auto scriptF = lnf.scriptFont (64.0f);
    const auto logoArea = juce::Rectangle<float> (40.0f, 8.0f, 400.0f, 84.0f);
    g.setFont (scriptF);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawText ("Klangbild", logoArea.translated (2.0f, 3.0f), juce::Justification::centredLeft);
    g.setColour (logo);
    g.drawText ("Klangbild", logoArea, juce::Justification::centredLeft);
    const float sw = (float) juce::GlyphArrangement::getStringWidthInt (scriptF, "Klangbild");
    g.setColour (gold);
    g.setFont (lnf.labelFont (26.0f, 0.35f));
    g.drawText ("FUZZ", juce::Rectangle<float> (logoArea.getX() + sw + 14.0f, logoArea.getY() + 30.0f, 140.0f, 40.0f), juce::Justification::centredLeft);
    g.setColour (logo.withAlpha (0.55f));
    g.setFont (lnf.labelFont (12.0f, 0.3f));
    g.drawText (u8 ("GERMANIUM \xc2\xb7 SILIZIUM \xc2\xb7 TONE BENDER \xc2\xb7 MUFF  +  BOOSTER"),
                juce::Rectangle<float> (44.0f, 90.0f, 600.0f, 16.0f), juce::Justification::centredLeft);
    const auto pa = presetArea.toFloat();
    g.setColour (logo.withAlpha (0.6f));
    g.setFont (lnf.labelFont (10.0f, 0.3f));
    g.drawText ("PROGRAMM", pa.withY (pa.getY() - 15.0f).withHeight (13.0f), juce::Justification::centredLeft);

    // Frontplatte
    const auto fp = faceplate.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (fp.expanded (4.0f).translated (0, 2.0f), 9.0f);
    g.setGradientFill (juce::ColourGradient (gold.brighter (0.3f), 0, fp.getY(), gold.darker (0.6f), 0, fp.getBottom(), false));
    g.fillRoundedRectangle (fp.expanded (3.0f), 8.0f);
    g.drawImageAt (faceplateImage, faceplate.getX(), faceplate.getY());
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (fp, 6.0f, 1.0f);
    for (auto [x, y, a] : { std::tuple { fp.getX() + 12.0f, fp.getY() + 12.0f, 0.4f }, std::tuple { fp.getRight() - 12.0f, fp.getY() + 12.0f, 1.3f },
                            std::tuple { fp.getX() + 12.0f, fp.getBottom() - 12.0f, 2.2f }, std::tuple { fp.getRight() - 12.0f, fp.getBottom() - 12.0f, 0.9f } })
        VintageLookAndFeel::drawScrew (g, { x, y }, 5.0f, a);

    // Rahmen um das Lampenfenster
    const auto lb = lampBezel.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (lb.translated (2.0f, 3.0f), 12.0f);
    g.setGradientFill (juce::ColourGradient (gold.brighter (0.25f), 0, lb.getY(), gold.darker (0.55f), 0, lb.getBottom(), false));
    g.fillRoundedRectangle (lb, 12.0f);
    for (auto pt : { lb.getTopLeft().translated (11, 11), lb.getTopRight().translated (-11, 11),
                     lb.getBottomLeft().translated (11, -11), lb.getBottomRight().translated (-11, -11) })
        VintageLookAndFeel::drawScrew (g, pt, 4.0f, pt.x * 0.013f);

    // Trennlinie
    const auto dv = divider.toFloat();
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawHorizontalLine ((int) dv.getY() + 1, dv.getX(), dv.getRight());
    g.setColour (ink.withAlpha (0.45f));
    g.drawHorizontalLine ((int) dv.getY(), dv.getX(), dv.getRight());

    for (auto& k : knobs)
    {
        const auto sb = k->slider.getBounds().toFloat();
        drawEngraved (g, k->name, { sb.getX() - 20.0f, sb.getBottom() - 4.0f, sb.getWidth() + 40.0f, k->big ? 24.0f : 18.0f },
                      lnf.labelFont (k->big ? 18.0f : 13.0f, k->big ? 0.2f : 0.14f), juce::Justification::centred);
    }
    const char* sl[] = { "FUZZ", "BOOSTER", "REIHENFOLGE", "OKTAVE" };
    for (int i = 0; i < 4; ++i)
        drawEngraved (g, sl[i], switchLabels[(size_t) i].toFloat(), lnf.labelFont (12.0f, 0.14f), juce::Justification::centredLeft);
    // Trenner zwischen Fuzz- und Booster-Reglern
    const auto bs = boosterSep.toFloat();
    g.setColour (ink.withAlpha (0.35f));
    g.drawVerticalLine ((int) bs.getX(), bs.getY(), bs.getBottom());
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.drawVerticalLine ((int) bs.getX() + 1, bs.getY(), bs.getBottom());

    const auto tag = juce::Rectangle<float> (120.0f, 24.0f).withPosition ((float) presetArea.getRight() + 24.0f, (float) presetArea.getY() + 4.0f);
    g.setColour (ink.withAlpha (0.85f));
    g.fillRoundedRectangle (tag, 3.0f);
    g.setColour (gold);
    g.setFont (lnf.labelFont (11.0f, 0.2f));
    g.drawText ("MODELL KB-66", tag, juce::Justification::centred);
}

void FuzzAudioProcessorEditor::resized()
{
    presetArea = { 44, 128, 300, 32 };
    presetBox.setBounds (presetArea);
    faceplate = { 24, 178, getWidth() - 48, getHeight() - 202 };
    const auto in = faceplate.reduced (34, 22);

    fuzzKnob->slider.setBounds (in.getX() + 20, in.getY() + 10, 230, 210);
    volKnob->slider.setBounds (in.getRight() - 250, in.getY() + 10, 230, 210);
    lampBezel = juce::Rectangle<int> (400, 196).withCentre ({ in.getCentreX(), in.getY() + 108 });
    curveWin.setBounds (lampBezel.reduced (16).withTrimmedBottom (26));
    typeLabel.setBounds (lampBezel.getX() + 16, lampBezel.getBottom() - 38, lampBezel.getWidth() - 32, 22);

    divider = { in.getX() + 10, in.getY() + 246, in.getWidth() - 20, 2 };

    const int ry = in.getY() + 262;
    constexpr int kw = 104, kh = 108;
    Knob* row[] = { typeKnob, biasKnob, toneKnob, guitarKnob, bTypeKnob, boostKnob };
    const int cell = 116;
    for (int i = 0; i < 6; ++i)
        row[i]->slider.setBounds (in.getX() + i * cell + (i >= 4 ? 24 : 0) + (cell - kw) / 2, ry, kw, kh);
    boosterSep = { in.getX() + 4 * cell + 10, ry + 4, 2, kh };

    const int sx = in.getX() + 6 * cell + 50;
    for (int i = 0; i < 4; ++i)
    {
        const int cx = sx + (i % 2) * 165, cy = ry + (i / 2) * 64;
        switchLabels[(size_t) i] = { cx, cy, 160, 16 };
    }
    fuzzSwitch.setBounds (sx, ry + 18, 150, 40);
    boostSwitch.setBounds (sx + 165, ry + 18, 150, 40);
    orderSwitch.setBounds (sx, ry + 82, 150, 40);
    octSwitch.setBounds (sx + 165, ry + 82, 150, 40);
    pilotArea = {};
    renderTextures();
}

//==============================================================================
void FuzzAudioProcessorEditor::CurveWindow::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff15110d), 0, b.getY(), juce::Colour (0xff060504), 0, b.getBottom(), false));
    g.fillRoundedRectangle (b, 4.0f);

    // zwei Transistoren (Metallgehäuse) links, glühen mit dem Signal
    const juce::Colour glow (0xffff9a3c);
    for (int k = 0; k < 2; ++k)
    {
        const auto can = juce::Rectangle<float> (30.0f, 30.0f).withCentre ({ b.getX() + 30.0f, b.getY() + 40.0f + (float) k * 62.0f });
        const float L = on ? juce::jlimit (0.0f, 1.0f, level * 3.0f) : 0.0f;
        g.setColour (glow.withAlpha (0.18f * L));
        g.fillEllipse (can.expanded (12.0f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd8d2c4), can.getX(), can.getY(), juce::Colour (0xff5a554c), can.getRight(), can.getBottom(), false));
        g.fillEllipse (can);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawEllipse (can, 1.0f);
        g.setColour (juce::Colour (0xff2b2722));
        g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
        g.drawText (type == 0 || type == 2 ? "GE" : "SI", can, juce::Justification::centred);
        for (int leg = -1; leg <= 1; ++leg)
        {
            g.setColour (juce::Colour (0xff9c9584));
            g.fillRect (can.getCentreX() + (float) leg * 6.0f - 0.7f, can.getBottom(), 1.4f, 12.0f);
        }
    }

    // Kennlinie
    auto plot = b.withTrimmedLeft (62.0f).reduced (10.0f, 12.0f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawHorizontalLine ((int) plot.getCentreY(), plot.getX(), plot.getRight());
    g.drawVerticalLine ((int) plot.getCentreX(), plot.getY(), plot.getBottom());
    g.drawRect (plot, 1.0f);
    const float range = 0.1f;    // Eingang ±0,1 (typischer Gitarrenpegel)
    juce::Path p;
    for (int i = 0; i <= 120; ++i)
    {
        const float x = -range + 2.0f * range * (float) i / 120.0f;
        const float y = on ? FuzzAudioProcessor::curve (type, x, fuzz, bias) : x / range;
        const float px = plot.getX() + plot.getWidth() * (float) i / 120.0f;
        const float py = plot.getCentreY() - juce::jlimit (-1.1f, 1.1f, y) * plot.getHeight() * 0.45f;
        if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
    }
    g.setColour (glow.withAlpha (0.3f));
    g.strokePath (p, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved));
    g.setColour (juce::Colour (0xffffd9a0));
    g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));
    // aktueller Pegel: Bereich, den die Gitarre gerade durchläuft
    const float lv = juce::jlimit (0.0f, range, level);
    const float w = plot.getWidth() * lv / (2.0f * range);
    g.setColour (glow.withAlpha (0.16f));
    g.fillRect (juce::Rectangle<float> (plot.getCentreX() - w, plot.getY(), 2.0f * w, plot.getHeight()));
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.setFont (juce::FontOptions (9.5f).withStyle ("Bold"));
    g.drawText ("EINGANG", plot.withHeight (12.0f).translated (0.0f, plot.getHeight() - 12.0f), juce::Justification::centredRight);
    g.drawText ("AUSGANG", plot.withHeight (12.0f).translated (4.0f, 0.0f), juce::Justification::centredLeft);

    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 2.0f);
}
