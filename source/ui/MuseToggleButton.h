#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "DesignTokens.h"

/**
 * MuseToggleButton - Custom button for FREEZE/DANGER controls
 *
 * Purpose: Toggle button with on/off states, linked to APVTS parameter
 * Visual: Dark when off, colored when on (green for FREEZE, red for DANGER)
 */
class MuseToggleButton : public juce::Component
{
public:
    MuseToggleButton(juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& parameterID,
                     const juce::String& labelText,
                     bool isDanger = false);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::String parameterID;
    juce::String labelText;
    bool isDanger;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MuseToggleButton)
};
