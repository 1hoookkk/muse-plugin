#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/STFTProcessor.h"
#include "dsp/FormantEnvelope.h"
#include "dsp/FreezeCapture.h"

#if (MSVC)
#include "ipps.h"
#endif

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Parameter access
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Parameters (APVTS manages automation and state)
    juce::AudioProcessorValueTreeState apvts;

    // DSP Components (M2: STFT freeze + envelope shaping)
    STFTProcessor stftProcessor;
    FormantEnvelope formantEnvelope;
    FreezeCapture freezeCapture;

    // Working buffers (preallocated, RT-safe)
    std::vector<float> workingMagnitudes;
    std::vector<float> envelopeBuffer;
    std::vector<float> tempInputBuffer;
    std::vector<float> tempOutputBuffer;

    // Parameter smoothing (for mix)
    float mixSmoothed = 0.5f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
