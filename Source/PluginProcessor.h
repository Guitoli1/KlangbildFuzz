#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "FuzzDsp.h"

class FuzzAudioProcessor : public juce::AudioProcessor
{
public:
    FuzzAudioProcessor();
    ~FuzzAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    struct Preset { const char* name; std::vector<std::pair<const char*, float>> values; };
    static const std::vector<Preset>& presets();
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Ausgangspegel je Fuzz-Typ (gemessen): bei Lautstärke 0 dB etwa −14 dBFS RMS – wie ein echtes Pedal heißer als die Gitarre
    static constexpr std::array<float, 4> outNorm { 0.31f, 0.27f, 0.25f, 0.476f };
    // Kennlinie der Fuzz-Stufe ohne Filter (für die Anzeige)
    static float curve (int type, float x, float fuzz01, float bias01);
    static float fuzzGain (int type, float fuzz01);

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> inputPeak { 0 }, vuOut { 0 }, peakOut { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void updateFilters (int type, float tone, float guitar);
    juce::dsp::Oversampling<float> os { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    std::array<fuzz::Channel, 2> chans;
    int lastType = -1, lastBoostType = -1; float lastTone = -1, lastGuitar = -1;
    juce::SmoothedValue<float> smFuzz, smBias, smBoost, smOut, smGuitar;
    float vuState = 0, vuCoef = 0.001f;
    int currentProgram = 0;
    double sr = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FuzzAudioProcessor)
};
