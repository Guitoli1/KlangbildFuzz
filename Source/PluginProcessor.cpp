#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ScaledEditor.h"

namespace
{
using APF = juce::AudioParameterFloat;
using Attr = juce::AudioParameterFloatAttributes;
Attr withText (std::function<juce::String (float)> f)
{
    return Attr().withStringFromValueFunction ([f] (float v, int) { return f (v); })
                 .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); });
}
Attr percent() { return withText ([] (float v) { return juce::String (juce::roundToInt (v)) + " %"; }); }
Attr db() { return withText ([] (float v) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; }); }
juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }
}

juce::AudioProcessorValueTreeState::ParameterLayout FuzzAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<APF> (juce::ParameterID { "fuzz", 1 }, "Fuzz", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 70.0f, percent()));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "type", 1 }, "Fuzz-Typ",
        juce::StringArray { "Germanium", "Silizium", "Tone Bender", "Muff" }, 0));
    l.add (std::make_unique<APF> (juce::ParameterID { "bias", 1 }, "Bias", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 70.0f, percent()));
    l.add (std::make_unique<APF> (juce::ParameterID { "tone", 1 }, "Ton", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, percent()));
    l.add (std::make_unique<APF> (juce::ParameterID { "guitar", 1 }, "Gitarren-Volume", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, percent()));
    l.add (std::make_unique<APF> (juce::ParameterID { "volume", 1 }, u8 ("Lautst\xc3\xa4rke"), juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, db()));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "fuzzOn", 1 }, "Fuzz an", true));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "octave", 1 }, "Oktave", false));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "boostType", 1 }, "Booster-Typ",
        juce::StringArray { "Clean", u8 ("H\xc3\xb6hen"), "Mitten" }, 1));
    l.add (std::make_unique<APF> (juce::ParameterID { "boost", 1 }, "Boost", juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 10.0f, db()));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "boostOn", 1 }, "Booster an", false));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "boostFirst", 1 }, "Booster vor Fuzz", true));
    return l;
}

const std::vector<FuzzAudioProcessor::Preset>& FuzzAudioProcessor::presets()
{
    //                       fuzz type bias tone guitar volume fuzzOn octave boostType boost boostOn boostFirst
    auto P = [] (const char* name, float fz, int ty, float bi, float to, float gu, float vo, bool fo, bool oc, int bt, float bo, bool bon, bool bf)
    {
        return Preset { name, { { "fuzz", fz }, { "type", (float) ty }, { "bias", bi }, { "tone", to }, { "guitar", gu }, { "volume", vo },
                                { "fuzzOn", fo ? 1.0f : 0.0f }, { "octave", oc ? 1.0f : 0.0f }, { "boostType", (float) bt }, { "boost", bo },
                                { "boostOn", bon ? 1.0f : 0.0f }, { "boostFirst", bf ? 1.0f : 0.0f } } };
    };
    static const std::vector<Preset> p = {
        P ("Standard",                                          70, 0, 70, 50, 100, 0,  true,  false, 1, 10, false, true),
        P ("Germanium \xe2\x80\x93 warm & rund",                 60, 0, 80, 40, 100, 0,  true,  false, 1, 10, false, true),
        P ("Germanium \xe2\x80\x93 aufgeklart (Volume 60 %)",   75, 0, 75, 55, 60,  4,  true,  false, 1, 10, false, true),
        P ("Silizium \xe2\x80\x93 Solo",                        80, 1, 70, 60, 100, 0,  true,  false, 1, 10, false, true),
        P ("Tone Bender \xe2\x80\x93 britisch",                  75, 2, 65, 55, 100, 0,  true,  false, 1, 10, false, true),
        P ("Muff \xe2\x80\x93 Wand aus Sustain",                85, 3, 70, 45, 100, 0,  true,  false, 1, 10, false, true),
        P ("Oktav-Fuzz",                                         80, 1, 70, 65, 100, 0,  true,  true,  1, 10, false, true),
        P ("Gespuckt \xe2\x80\x93 Bias niedrig",                80, 0, 12, 50, 100, 2,  true,  false, 1, 10, false, true),
        P ("H\xc3\xb6hen-Booster vor Germanium",                 55, 0, 75, 50, 100, 0,  true,  false, 1, 12, true,  true),
        P ("Nur Booster \xe2\x80\x93 H\xc3\xb6hen (vor den Amp)", 70, 0, 70, 50, 100, 0, false, false, 1, 12, true,  true),
        P ("Nur Booster \xe2\x80\x93 Mitten (Solo)",             70, 0, 70, 50, 100, 0,  false, false, 2, 9,  true,  true),
        P ("Fuzz + Solo-Boost dahinter",                         75, 0, 70, 50, 100, 0,  true,  false, 0, 6,  true,  false),
    };
    return p;
}

void FuzzAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets().size()))
        return;
    currentProgram = index;
    for (auto& [id, value] : presets()[(size_t) index].values)
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
}

const juce::String FuzzAudioProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, (int) presets().size())
               ? juce::String (juce::CharPointer_UTF8 (presets()[(size_t) index].name)) : juce::String();
}

FuzzAudioProcessor::FuzzAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KlangbildFuzz", createLayout())
{
}

bool FuzzAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

//==============================================================================
float FuzzAudioProcessor::fuzzGain (int type, float f)
{
    static constexpr float lo[] { 8.0f, 14.0f, 10.0f, 4.0f }, hi[] { 46.0f, 54.0f, 44.0f, 38.0f };
    const int t = juce::jlimit (0, 3, type);
    return juce::Decibels::decibelsToGain (lo[t] + (hi[t] - lo[t]) * juce::jlimit (0.0f, 1.0f, f));
}

float FuzzAudioProcessor::curve (int type, float x, float f, float bias)
{
    const float g = fuzzGain (type, f);
    switch (type)
    {
        case fuzz::silicon: return fuzz::siliconStage (x, g, bias);
        case fuzz::bender:  return fuzz::germaniumStage (fuzz::germaniumStage (x, std::sqrt (g) * 2.0f, bias) * 0.5f, std::sqrt (g) * 1.5f, bias);
        case fuzz::muff:    return fuzz::diode (fuzz::diode (x * 10.0f) * g * 0.5f);
        default:            return fuzz::germaniumStage (x, g, bias);
    }
}

void FuzzAudioProcessor::updateFilters (int type, float tone, float guitar)
{
    const double osr = sr * 4.0;
    for (auto& c : chans)
    {
        static constexpr double inHp[] { 40.0, 60.0, 110.0, 90.0 };
        c.inHp.set (osr, inHp[type]);
        c.preLp.set (osr, 9000.0);
        c.postLp1.set (osr, type == fuzz::silicon ? 9000.0 : 6000.0);
        c.postLp2.set (osr, type == fuzz::muff ? 7000.0 : 11000.0);
        c.octHp.set (osr, 80.0);
        if (type == fuzz::muff) { c.toneLp.set (osr, 480.0); c.toneHp.set (osr, 1100.0); }
        else c.toneLp.set (osr, 1300.0 * std::pow (10.0, (double) tone));   // 1,3 … 13 kHz
        c.benderPeak.set (osr, 1000.0, 0.7, 7.0);
        c.boostHp.set (osr, 900.0);
        c.boostPeak.set (osr, 720.0, 0.9, 10.0);
        c.boostLp.set (osr, 150.0);
        c.dcOut.set (osr, 20.0);
        // Gitarren-Poti: zurückgedreht wird es etwas dumpfer (Kabelkapazität)
        c.guitarLp.set (osr, 3500.0 + 16500.0 * guitar);
    }
}

void FuzzAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    os.initProcessing ((size_t) juce::jmax (1, samplesPerBlock));
    os.reset();
    setLatencySamples ((int) std::round (os.getLatencyInSamples()));
    for (auto& c : chans) c.reset();
    lastType = -1;
    for (auto* s : { &smFuzz, &smBias, &smBoost, &smOut, &smGuitar })
        s->reset (sr * 4.0, 0.03);
    smFuzz.setCurrentAndTargetValue (apvts.getRawParameterValue ("fuzz")->load() * 0.01f);
    smBias.setCurrentAndTargetValue (apvts.getRawParameterValue ("bias")->load() * 0.01f);
    smBoost.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("boost")->load()));
    smOut.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("volume")->load()));
    smGuitar.setCurrentAndTargetValue (apvts.getRawParameterValue ("guitar")->load() * 0.01f);
    vuCoef = (float) (1.0 - std::exp (-1.0 / (0.01 * sr)));
    vuState = 0;
}

void FuzzAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels(), outCh = juce::jmin (2, getTotalNumOutputChannels());
    if (n == 0 || outCh == 0)
        return;
    if (inCh == 1 && outCh == 2)
        buffer.copyFrom (1, 0, buffer, 0, 0, n);

    const int type = juce::jlimit (0, 3, (int) apvts.getRawParameterValue ("type")->load());
    const int bType = juce::jlimit (0, 2, (int) apvts.getRawParameterValue ("boostType")->load());
    const float tone = apvts.getRawParameterValue ("tone")->load() * 0.01f;
    const float guitar = apvts.getRawParameterValue ("guitar")->load() * 0.01f;
    if (type != lastType || ! juce::exactlyEqual (tone, lastTone) || ! juce::exactlyEqual (guitar, lastGuitar))
    {
        updateFilters (type, tone, guitar);
        lastType = type; lastTone = tone; lastGuitar = guitar;
    }
    const bool fuzzOn = apvts.getRawParameterValue ("fuzzOn")->load() > 0.5f;
    const bool boostOn = apvts.getRawParameterValue ("boostOn")->load() > 0.5f;
    const bool boostFirst = apvts.getRawParameterValue ("boostFirst")->load() > 0.5f;
    const bool octave = apvts.getRawParameterValue ("octave")->load() > 0.5f;
    smFuzz.setTargetValue (apvts.getRawParameterValue ("fuzz")->load() * 0.01f);
    smBias.setTargetValue (apvts.getRawParameterValue ("bias")->load() * 0.01f);
    smBoost.setTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("boost")->load()));
    smOut.setTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("volume")->load()) * (fuzzOn ? outNorm[(size_t) type] : 1.0f));
    smGuitar.setTargetValue (guitar);

    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) outCh, (size_t) n);
    auto up = os.processSamplesUp (block);
    const int m = (int) up.getNumSamples();
    float inPk = 0;

    for (int i = 0; i < m; ++i)
    {
        const float fz = smFuzz.getNextValue(), bias = smBias.getNextValue(), bg = smBoost.getNextValue();
        const float og = smOut.getNextValue(), gv = smGuitar.getNextValue();
        const float vol = gv * gv * (2.0f - gv);   // Audio-Poti-Verlauf
        // Germanium-Fuzz und Tone Bender klaren auf, wenn das Gitarren-Volume zurückgeht
        // (die niedrige Eingangsimpedanz belastet das Poti – die Verstärkung sinkt mit)
        const float g = fuzzGain (type, fz) * ((type == fuzz::germanium || type == fuzz::bender) ? std::pow (vol, 1.5f) : 1.0f);
        for (int c = 0; c < outCh; ++c)
        {
            auto& ch = chans[(size_t) c];
            float x = ch.guitarLp.lp (up.getSample (c, i) * vol);
            if (c == 0) inPk = juce::jmax (inPk, std::abs (x));

            auto booster = [&] (float s)
            {
                if (! boostOn) return s;
                switch (bType)
                {
                    case fuzz::treble: { const float h = ch.boostHp.hp (s); return std::tanh ((0.25f * s + h) * bg * 0.8f) / 0.8f; }
                    case fuzz::mid:    { const float p = ch.boostPeak.process (s); const float h = p - ch.boostLp.lp (p); return std::tanh (h * 0.6f * bg * 0.6f) / 0.6f; }
                    default:           return std::tanh (s * bg * 0.6f) / 0.6f;
                }
            };
            auto fuzzer = [&] (float s)
            {
                if (! fuzzOn) return s;
                s = ch.inHp.hp (s);
                if (octave) s += 0.9f * ch.octHp.hp (std::abs (s)) * 1.6f;   // Gleichrichter: Oktave nach oben
                float y;
                switch (type)
                {
                    case fuzz::silicon: y = fuzz::siliconStage (s, g, bias); break;
                    case fuzz::bender:
                    {
                        const float e = ch.benderPeak.process (s);
                        y = fuzz::germaniumStage (fuzz::germaniumStage (e, std::sqrt (g) * 2.0f, bias) * 0.5f, std::sqrt (g) * 1.5f, bias);
                        break;
                    }
                    case fuzz::muff:
                    {
                        const float s1 = fuzz::diode (ch.preLp.lp (s) * 10.0f);
                        const float s2 = fuzz::diode (s1 * g * 0.5f);
                        const float lo = ch.toneLp.lp (s2), hi = ch.toneHp.hp (s2);
                        y = 1.8f * (lo * (1.0f - tone) + hi * tone);   // Mitten-Senke, Ton blendet tief ↔ hell
                        break;
                    }
                    default: y = fuzz::germaniumStage (s, g, bias); break;
                }
                y = ch.postLp2.lp (ch.postLp1.lp (y));
                if (type != fuzz::muff) y = ch.toneLp.lp (y);
                return y;
            };
            float y = boostFirst ? fuzzer (booster (x)) : booster (fuzzer (x));
            y = ch.dcOut.hp (y) * og;
            up.setSample (c, i, y);
        }
    }
    os.processSamplesDown (block);

    float pk = 0;
    for (int i = 0; i < n; ++i)
    {
        const float y = buffer.getSample (0, i);
        pk = juce::jmax (pk, std::abs (y));
        vuState += (std::abs (y) - vuState) * vuCoef;
    }
    inputPeak.store (juce::jmax (inPk, inputPeak.load() * 0.92f));
    vuOut.store (vuState);
    peakOut.store (juce::jmax (pk, peakOut.load()));
}

void FuzzAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void FuzzAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;
    auto tree = juce::ValueTree::fromXml (*xml);
    currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) tree.getProperty ("program", 0));
    apvts.replaceState (tree);
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->convertTo0to1 (rp->convertFrom0to1 (rp->getValue())));
}

juce::AudioProcessorEditor* FuzzAudioProcessor::createEditor() { return new kb::ScaledEditor (*this, new FuzzAudioProcessorEditor (*this)); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FuzzAudioProcessor(); }
