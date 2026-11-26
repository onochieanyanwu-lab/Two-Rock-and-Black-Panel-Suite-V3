#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include "AmpCore.h"

#if defined(AMP_TWOROCK)
 inline constexpr bool kIsTwoRock = true;
#else
 inline constexpr bool kIsTwoRock = false;
#endif

class AmpProcessor : public juce::AudioProcessor
{
public:
    AmpProcessor();
    ~AmpProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    {
        return l.getMainOutputChannelSet() == l.getMainInputChannelSet()
            && (l.getMainOutputChannelSet() == juce::AudioChannelSet::mono() || l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
    }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return kIsTwoRock ? "Two Rock Classic Reverb Signature" : "Blackface Fender"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void loadIR (const juce::File& f);
    juce::String getIRName() const { return irPath.isEmpty() ? juce::String ("none") : juce::File (irPath).getFileName(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void rebuildEngine();
    float pv (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    std::unique_ptr<amp::AmpEngine> engine = std::make_unique<amp::AmpEngine>();
    std::unique_ptr<juce::dsp::Oversampling<float>> os2, os4;
    juce::dsp::Oversampling<float>* os = nullptr;
    amp::CabSim cab;
    juce::dsp::Convolution convolution;
    juce::AudioBuffer<float> mono;
    juce::String irPath;

    double hostRate = 44100.0; int maxBlock = 512;
    int appliedQuality = -1, appliedModel = -1;
    amp::Controls cur;
    bool controlsInit = false;
    float lastOutGain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmpProcessor)
};
