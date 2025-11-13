#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "DesignTokens.h"

/**
 * BeveledKnob - Hardware-style beveled knob with mint indicator
 *
 * Purpose: 72×72px knob for MORPH/INTENSITY/MIX parameters
 * Features:
 * - Radial gradient bezel with 3D inset shadows
 * - Rotating mint indicator line
 * - Vertical drag interaction (150px = full range)
 * - Double-click to reset to 0.5
 * - OLED text labels (above) and value (below)
 */
class BeveledKnob : public juce::Component
{
public:
    BeveledKnob(juce::AudioProcessorValueTreeState& apvts,
                const juce::String& parameterID,
                const juce::String& labelText);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    /**
     * Get current normalized value [0.0, 1.0]
     */
    float getValue() const;

    /**
     * Set value and notify host
     */
    void setValue(float newValue);

    /**
     * Get label text for OLED rendering
     */
    juce::String getLabelText() const { return labelText; }

    /**
     * Get formatted value string (e.g., "0.5")
     */
    juce::String getValueText() const;

private:
    void drawOLEDText(juce::Graphics& g, const juce::String& text,
                     juce::Rectangle<float> bounds,
                     juce::Justification justification);

    juce::AudioProcessorValueTreeState& apvts;
    juce::String parameterID;
    juce::String labelText;

    float dragStartValue = 0.0f;
    int dragStartY = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BeveledKnob)
};
