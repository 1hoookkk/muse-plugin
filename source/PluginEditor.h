#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include "ui/DesignTokens.h"
#include "ui/ChassisTexture.h"
#include "ui/BeveledKnob.h"
#include "ui/MuseToggleButton.h"
#include "ui/StatusLED.h"
#include "ui/HalftoneMouth.h"

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    void drawOLEDText(juce::Graphics& g, const juce::String& text,
                     juce::Rectangle<float> bounds,
                     juce::Justification justification);

    void drawLCDPanel(juce::Graphics& g);
    void drawTitle(juce::Graphics& g);
    void drawPairBadge(juce::Graphics& g);
    void drawSerialNumber(juce::Graphics& g);
    void drawKnobLabels(juce::Graphics& g);
    void drawLEDLabel(juce::Graphics& g);

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PluginProcessor& processorRef;

    // Design system
    ChassisTexture chassisTexture;

    // UI Components
    BeveledKnob morphKnob;
    BeveledKnob intensityKnob;
    BeveledKnob mixKnob;
    MuseToggleButton freezeButton;
    MuseToggleButton dangerButton;
    StatusLED statusLED;
    HalftoneMouth halftoneMouth;

    // Inspector (dev tool)
    std::unique_ptr<melatonin::Inspector> inspector;
    juce::TextButton inspectButton { "Inspect the UI" };

    // Timer state
    int frameCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
