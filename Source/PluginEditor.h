#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "VintageUI.h"

class FuzzAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit FuzzAudioProcessorEditor (FuzzAudioProcessor&);
    ~FuzzAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // Sichtfenster: Kennlinie der Fuzz-Stufe mit dem aktuellen Eingangspegel
    struct CurveWindow : public juce::Component
    {
        int type = 0; float fuzz = 0.7f, bias = 0.7f, level = 0; bool on = true;
        void paint (juce::Graphics&) override;
    };

    struct Knob { juce::Slider slider; juce::String name; bool big = false; std::unique_ptr<SliderAttachment> attachment; };
    Knob& addKnob (const juce::String& paramID, const juce::String& name, bool big);
    void setupSwitch (juce::ToggleButton&, const juce::String& paramID, const juce::String& onText,
                      const juce::String& offText, juce::Colour lampColour);
    void timerCallback() override;
    void renderTextures();
    void drawEngraved (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Font, juce::Justification);

    FuzzAudioProcessor& proc;
    VintageLookAndFeel lnf;
    std::vector<std::unique_ptr<Knob>> knobs;
    Knob* fuzzKnob = nullptr; Knob* volKnob = nullptr;
    Knob* typeKnob = nullptr; Knob* biasKnob = nullptr; Knob* toneKnob = nullptr; Knob* guitarKnob = nullptr; Knob* bTypeKnob = nullptr; Knob* boostKnob = nullptr;
    juce::ToggleButton fuzzSwitch, boostSwitch, orderSwitch, octSwitch;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    juce::ComboBox presetBox;
    CurveWindow curveWin;
    juce::Label typeLabel;

    juce::Rectangle<int> faceplate, presetArea, lampBezel, pilotArea, divider;
    juce::Rectangle<int> boosterSep;
    std::array<juce::Rectangle<int>, 4> switchLabels;
    juce::Image tolexImage, faceplateImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FuzzAudioProcessorEditor)
};
