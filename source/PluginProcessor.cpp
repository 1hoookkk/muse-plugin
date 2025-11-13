#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
                    #if ! JucePlugin_IsMidiEffect
                     #if ! JucePlugin_IsSynth
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     #endif
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                    #endif
                      ),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

PluginProcessor::~PluginProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // MORPH (0.0 - 1.0, default 0.5, smoothed)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "morph", 1 },
        "Morph",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.5f));

    // INTENSITY (0.0 - 1.0, default 0.5, smoothed)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "intensity", 1 },
        "Intensity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.5f));

    // MIX (0.0 - 1.0, default 0.5, smoothed)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 },
        "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.5f));

    // PAIR (0 - 3, default 0, discrete)
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "pair", 1 },
        "Pair",
        0, 3, 0));

    // AUTO (boolean, default false) - Actually used for FREEZE in UI
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "auto", 1 },
        "Freeze",
        false));

    // DANGER (boolean, default false)
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "danger", 1 },
        "Danger",
        false));

    return layout;
}

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    // M2: Report STFT latency for DAW compensation
    return stftProcessor.getLatencySamples() / getSampleRate();
}

int PluginProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // M2: Initialize STFT-based spectral freeze processor
    stftProcessor.prepare(sampleRate, samplesPerBlock);

    // Initialize freeze capture
    const int numBins = stftProcessor.getNumBins();
    freezeCapture.prepare(numBins, sampleRate, STFTProcessor::HOP_SIZE);

    // M4: Initialize RMS trackers
    inputRMSTracker.prepare(sampleRate, STFTProcessor::HOP_SIZE);
    outputRMSTracker.prepare(sampleRate, STFTProcessor::HOP_SIZE);

    // Allocate working buffers (RT-safe after this)
    workingMagnitudes.resize(numBins, 0.0f);
    envelopeBuffer.resize(numBins, 0.0f);
    tempInputBuffer.resize(samplesPerBlock, 0.0f);
    tempOutputBuffer.resize(samplesPerBlock, 0.0f);

    // Initialize parameter smoothing (M3: add morph, intensity)
    mixSmoothed = apvts.getRawParameterValue("mix")->load();
    morphSmoothed = apvts.getRawParameterValue("morph")->load();
    intensitySmoothed = apvts.getRawParameterValue("intensity")->load();

    // M4: Initialize adaptive gain
    adaptiveGain = 1.0f;
    adaptiveGainSmoothed = 1.0f;
}

void PluginProcessor::releaseResources()
{
    // Reset DSP state
    stftProcessor.reset();
    freezeCapture.reset();
    inputRMSTracker.reset();
    outputRMSTracker.reset();
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    const int numSamples = buffer.getNumSamples();

    // Clear extra output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    // M2/M3/M4: STFT freeze + envelope shaping + adaptive gain

    // Read parameters (atomic, lock-free)
    float mix = apvts.getRawParameterValue("mix")->load();
    bool freeze = apvts.getRawParameterValue("auto")->load() > 0.5f;
    float morph = apvts.getRawParameterValue("morph")->load();
    float intensity = apvts.getRawParameterValue("intensity")->load();
    int pair = static_cast<int>(apvts.getRawParameterValue("pair")->load());
    bool danger = apvts.getRawParameterValue("danger")->load() > 0.5f;

    // Smooth continuous parameters (20ms smoothing, ~0.95 coefficient)
    const float smoothCoeff = 0.95f;
    mixSmoothed = mixSmoothed * smoothCoeff + mix * (1.0f - smoothCoeff);
    morphSmoothed = morphSmoothed * smoothCoeff + morph * (1.0f - smoothCoeff);
    intensitySmoothed = intensitySmoothed * smoothCoeff + intensity * (1.0f - smoothCoeff);

    // M3: Get vowel shape for current pair and morph position
    VowelShape vowel = FormantEnvelope::getVowelForPair(pair, morphSmoothed);

    // Generate formant envelope with actual intensity
    formantEnvelope.generateEnvelope(envelopeBuffer, vowel, intensitySmoothed,
                                     getSampleRate(), STFTProcessor::FFT_SIZE);

    // M4: Track input RMS (measure before processing)
    float inputRMS = 0.0f;
    if (totalNumInputChannels > 0)
    {
        inputRMS = inputRMSTracker.computeRMS(buffer.getReadPointer(0), numSamples);
    }

    // Process each channel independently (stereo-linked DSP, but separate buffers)
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        // Input sanitization
        for (int i = 0; i < numSamples; ++i)
        {
            if (std::isnan(channelData[i]) || std::isinf(channelData[i]))
                channelData[i] = 0.0f;
        }

        // Copy input for dry signal
        std::copy(channelData, channelData + numSamples, tempInputBuffer.begin());

        // STFT processing with spectral callback
        stftProcessor.processBlock(
            channelData,
            tempOutputBuffer.data(),
            numSamples,
            [this, freeze](float* magnitudes, float* phases, int numBins) {
                // Freeze capture (captures/holds/crossfades spectrum)
                freezeCapture.processSpectrum(magnitudes, workingMagnitudes.data(),
                                             numBins, freeze);

                // Apply formant envelope
                for (int i = 0; i < numBins; ++i)
                {
                    magnitudes[i] = workingMagnitudes[i] * envelopeBuffer[i];

                    // Sanitize output
                    if (std::isnan(magnitudes[i]) || std::isinf(magnitudes[i]))
                        magnitudes[i] = 0.0f;
                }
            }
        );

        // M4: Measure wet output RMS and apply adaptive gain
        float wetRMS = outputRMSTracker.computeRMS(tempOutputBuffer.data(), numSamples);

        // Calculate adaptive gain (once per block, not per channel)
        if (channel == 0)
        {
            if (danger)
            {
                // Danger mode: Fixed +3 dB boost (sqrt(2) ≈ 1.414)
                adaptiveGain = 1.41421356f;
            }
            else
            {
                // Normal mode: Adaptive gain to match input RMS
                if (wetRMS > 0.001f)  // Avoid division by zero
                {
                    adaptiveGain = inputRMS / wetRMS;

                    // Clamp to reasonable range (prevent extreme corrections)
                    adaptiveGain = std::max(0.1f, std::min(10.0f, adaptiveGain));
                }
                else
                {
                    adaptiveGain = 1.0f;  // No signal, unity gain
                }
            }

            // Smooth adaptive gain (prevent pumping artifacts)
            const float gainSmoothCoeff = 0.98f;  // ~100-200ms
            adaptiveGainSmoothed = adaptiveGainSmoothed * gainSmoothCoeff +
                                  adaptiveGain * (1.0f - gainSmoothCoeff);

            // Write RMS level to atomic for UI feedback
            rmsLevelForUI.store(wetRMS, std::memory_order_relaxed);
        }

        // Apply adaptive gain to wet signal BEFORE mixing
        for (int i = 0; i < numSamples; ++i)
        {
            tempOutputBuffer[i] *= adaptiveGainSmoothed;

            // Sanitize after gain
            if (std::isnan(tempOutputBuffer[i]) || std::isinf(tempOutputBuffer[i]))
                tempOutputBuffer[i] = 0.0f;
        }

        // Mix dry/wet with equal-power crossfade
        float wetGain = std::sin(mixSmoothed * juce::MathConstants<float>::halfPi);
        float dryGain = std::cos(mixSmoothed * juce::MathConstants<float>::halfPi);

        for (int i = 0; i < numSamples; ++i)
        {
            channelData[i] = tempInputBuffer[i] * dryGain + tempOutputBuffer[i] * wetGain;

            // Final sanitization
            if (std::isnan(channelData[i]) || std::isinf(channelData[i]))
                channelData[i] = 0.0f;
        }
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Save parameters using APVTS
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore parameters using APVTS
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
