#include "MuseToggleButton.h"

MuseToggleButton::MuseToggleButton(juce::AudioProcessorValueTreeState& apvts_,
                                   const juce::String& parameterID_,
                                   const juce::String& labelText_,
                                   bool isDanger_)
    : apvts(apvts_), parameterID(parameterID_), labelText(labelText_), isDanger(isDanger_)
{
    // No attachment needed - we'll read/write parameter directly in mouseDown
}

void MuseToggleButton::paint(juce::Graphics& g)
{
    auto& tokens = DesignTokens::getInstance();
    auto bounds = getLocalBounds().toFloat();

    // Read current parameter state
    float paramValue = apvts.getRawParameterValue(parameterID)->load();
    bool isOn = paramValue > 0.5f;

    // Background color
    juce::Colour bgColor;
    juce::Colour textColor;

    if (isOn)
    {
        bgColor = tokens.getButtonOnColor(isDanger);
        textColor = juce::Colours::white;
    }
    else
    {
        bgColor = juce::Colour(0xff1a2626);  // Dark background
        textColor = tokens.getOLEDMint().withAlpha(0.5f);
    }

    // Draw rounded rectangle background
    g.setColour(bgColor);
    g.fillRoundedRectangle(bounds, 2.0f);

    // Draw border (subtle)
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 2.0f, 1.0f);

    // Draw label text
    g.setColour(textColor);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText(labelText, bounds, juce::Justification::centred, false);
}

void MuseToggleButton::mouseDown(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);

    // Toggle parameter
    auto* param = apvts.getRawParameterValue(parameterID);
    if (param != nullptr)
    {
        float currentValue = param->load();
        float newValue = currentValue > 0.5f ? 0.0f : 1.0f;

        // Update parameter via APVTS (triggers automation recording)
        if (auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(parameterID)))
        {
            boolParam->setValueNotifyingHost(newValue);
        }

        repaint();
    }
}
