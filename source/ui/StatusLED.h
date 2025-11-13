#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "DesignTokens.h"

/**
 * StatusLED - Visual indicator with glow halo
 *
 * Purpose: Display audio level state (FLOW/STRUGGLE/MELTDOWN)
 * Features: Color-coded circle with glow, text label
 */
class StatusLED : public juce::Component
{
public:
    StatusLED();

    /**
     * Update LED state based on RMS level
     * @param rmsLevel Current RMS level (0.0 - 1.0)
     */
    void updateLevel(float rmsLevel);

    /**
     * Get current state name (for label text)
     */
    juce::String getStateName() const;

    void paint(juce::Graphics& g) override;

private:
    DesignTokens::LEDState currentState = DesignTokens::LEDState::FLOW;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusLED)
};
