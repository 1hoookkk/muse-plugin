#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "dsp/STFTProcessor.h"
#include "dsp/FormantEnvelope.h"
#include "dsp/FreezeCapture.h"
#include "dsp/RMSTracker.h"

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

    // UI feedback (M4: RMS level for LED status)
    float getCurrentRMSLevel() const { return rmsLevelForUI.load(std::memory_order_relaxed); }

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Parameters (APVTS manages automation and state)
    juce::AudioProcessorValueTreeState apvts;

    // DSP Components (M2: STFT freeze + envelope shaping)
    STFTProcessor stftProcessor;
    FormantEnvelope formantEnvelope;
    FreezeCapture freezeCapture;

    // M4: RMS tracking for adaptive gain
    RMSTracker inputRMSTracker;
    RMSTracker outputRMSTracker;

    // Working buffers (preallocated, RT-safe)
    std::vector<float> workingMagnitudes;
    std::vector<float> envelopeBuffer;
    std::vector<float> tempInputBuffer;
    std::vector<float> tempOutputBuffer;

    // Parameter smoothing (M3: add morph, intensity)
    float mixSmoothed = 0.5f;
    float morphSmoothed = 0.5f;
    float intensitySmoothed = 0.5f;

    // M4: Adaptive gain state
    float adaptiveGain = 1.0f;
    float adaptiveGainSmoothed = 1.0f;

    // M4: RMS level for UI (atomic communication)
    std::atomic<float> rmsLevelForUI { 0.0f };

    // M6: AUTO mode (spectral analysis for pair suggestion)
    int analyzeBandEnergy(const float* magnitudes, int numBins, double sampleRate);
    int suggestedPair = 0;
    int suggestedPairStable = 0;
    int pairSuggestionCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
