#include "StatusLED.h"

StatusLED::StatusLED()
{
}

void StatusLED::updateLevel(float rmsLevel)
{
    // Determine state based on RMS thresholds
    DesignTokens::LEDState newState;

    if (rmsLevel < 0.5f)  // < -6 dBFS
        newState = DesignTokens::LEDState::FLOW;
    else if (rmsLevel < 0.85f)  // -6 to -1.5 dBFS
        newState = DesignTokens::LEDState::STRUGGLE;
    else  // >= -1.5 dBFS
        newState = DesignTokens::LEDState::MELTDOWN;

    if (newState != currentState)
    {
        currentState = newState;
        repaint();
    }
}

juce::String StatusLED::getStateName() const
{
    switch (currentState)
    {
        case DesignTokens::LEDState::FLOW:
            return "FLOW";
        case DesignTokens::LEDState::STRUGGLE:
            return "STRUGGLE";
        case DesignTokens::LEDState::MELTDOWN:
            return "MELTDOWN";
        default:
            return "FLOW";
    }
}

void StatusLED::paint(juce::Graphics& g)
{
    auto& tokens = DesignTokens::getInstance();
    auto bounds = getLocalBounds().toFloat();

    // Get LED color for current state
    juce::Colour ledColor = tokens.getLEDColor(currentState);

    // Draw glow halo (4px expansion, alpha=0.25)
    auto haloBounds = bounds.expanded(4.0f);
    g.setColour(ledColor.withAlpha(0.25f));
    g.fillEllipse(haloBounds);

    // Draw main LED circle
    g.setColour(ledColor);
    g.fillEllipse(bounds);

    // Draw highlight (top-left, simulating light reflection)
    auto highlightBounds = bounds.reduced(2.0f).translated(-1.0f, -1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.fillEllipse(highlightBounds.withWidth(bounds.getWidth() * 0.4f)
                                  .withHeight(bounds.getHeight() * 0.4f));
}
