#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_data_structures/juce_data_structures.h>

/**
 * DesignTokens - Loads and provides access to design/design-tokens.json
 *
 * Purpose: Central source of truth for colors, fonts, layout positions
 * Usage: Singleton pattern, call getInstance() to access
 */
class DesignTokens
{
public:
    enum class LEDState
    {
        FLOW,
        STRUGGLE,
        MELTDOWN
    };

    DesignTokens();

    // Singleton access
    static DesignTokens& getInstance();

    // Colors
    juce::Colour getChassisColor() const { return chassisColor; }
    juce::Colour getLCDBackground() const { return lcdBackground; }
    juce::Colour getOLEDMint() const { return oledMint; }
    juce::Colour getMouthDotColor() const { return mouthDotColor; }
    juce::Colour getKnobGradientLight() const { return knobGradientLight; }
    juce::Colour getKnobGradientDark() const { return knobGradientDark; }
    juce::Colour getKnobInsetLight() const { return knobInsetLight; }
    juce::Colour getKnobInsetDark() const { return knobInsetDark; }

    juce::Colour getLEDColor(LEDState state) const;
    juce::Colour getButtonOnColor(bool isDanger) const;

    // Fonts
    juce::Font getTitleFont() const;
    juce::Font getLabelFont() const;
    juce::Font getValueFont() const;
    juce::Font getStatusFont() const;
    juce::Font getSerialFont() const;

    // Layout positions
    juce::Rectangle<int> getWindowBounds() const { return windowBounds; }
    juce::Rectangle<int> getLCDPanelBounds() const { return lcdPanelBounds; }
    juce::Rectangle<int> getHalftoneMouthBounds() const { return halftoneMouthBounds; }
    juce::Rectangle<int> getTitleBounds() const { return titleBounds; }
    juce::Rectangle<int> getStatusLEDBounds() const { return statusLEDBounds; }
    juce::Rectangle<int> getLEDLabelBounds() const { return ledLabelBounds; }
    juce::Rectangle<int> getMorphKnobBounds() const { return morphKnobBounds; }
    juce::Rectangle<int> getIntensityKnobBounds() const { return intensityKnobBounds; }
    juce::Rectangle<int> getMixKnobBounds() const { return mixKnobBounds; }
    juce::Rectangle<int> getAutoButtonBounds() const { return autoButtonBounds; }
    juce::Rectangle<int> getDangerButtonBounds() const { return dangerButtonBounds; }
    juce::Rectangle<int> getPairBadgeBounds() const { return pairBadgeBounds; }
    juce::Rectangle<int> getSerialNumberBounds() const { return serialNumberBounds; }

    // Timing
    int getMouthFPS() const { return mouthFPS; }
    int getKnobFPS() const { return knobFPS; }

private:
    void loadFromJSON();
    juce::Colour parseColor(const juce::String& hexString) const;

    // Cached colors
    juce::Colour chassisColor;
    juce::Colour lcdBackground;
    juce::Colour oledMint;
    juce::Colour mouthDotColor;
    juce::Colour knobGradientLight;
    juce::Colour knobGradientDark;
    juce::Colour knobInsetLight;
    juce::Colour knobInsetDark;
    juce::Colour ledFlow;
    juce::Colour ledStruggle;
    juce::Colour ledMeltdown;
    juce::Colour buttonAutoOn;
    juce::Colour buttonDangerOn;

    // Cached layout bounds
    juce::Rectangle<int> windowBounds;
    juce::Rectangle<int> lcdPanelBounds;
    juce::Rectangle<int> halftoneMouthBounds;
    juce::Rectangle<int> titleBounds;
    juce::Rectangle<int> statusLEDBounds;
    juce::Rectangle<int> ledLabelBounds;
    juce::Rectangle<int> morphKnobBounds;
    juce::Rectangle<int> intensityKnobBounds;
    juce::Rectangle<int> mixKnobBounds;
    juce::Rectangle<int> autoButtonBounds;
    juce::Rectangle<int> dangerButtonBounds;
    juce::Rectangle<int> pairBadgeBounds;
    juce::Rectangle<int> serialNumberBounds;

    // Timing
    int mouthFPS = 10;
    int knobFPS = 60;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DesignTokens)
};
