#include "DesignTokens.h"

DesignTokens::DesignTokens()
{
    loadFromJSON();
}

DesignTokens& DesignTokens::getInstance()
{
    static DesignTokens instance;
    return instance;
}

void DesignTokens::loadFromJSON()
{
    // Load design tokens from JSON file
    juce::File tokensFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                                .getParentDirectory()
                                .getParentDirectory()
                                .getParentDirectory()
                                .getParentDirectory()
                                .getChildFile("design/design-tokens.json");

    // Fallback: try relative to source directory (for development builds)
    if (!tokensFile.existsAsFile())
    {
        tokensFile = juce::File::getCurrentWorkingDirectory()
                         .getChildFile("design/design-tokens.json");
    }

    if (tokensFile.existsAsFile())
    {
        juce::String jsonString = tokensFile.loadFileAsString();
        juce::var parsedJSON = juce::JSON::parse(jsonString);

        if (parsedJSON.isObject())
        {
            auto root = parsedJSON.getProperty("muse", juce::var());

            // Parse colors
            auto colors = root.getProperty("colors", juce::var());
            chassisColor = parseColor(colors.getProperty("chassis", juce::var()).getProperty("moss", "#2F4F4F").toString());
            lcdBackground = parseColor(colors.getProperty("lcd", juce::var()).getProperty("background", "#F1F4F5").toString());
            oledMint = parseColor(colors.getProperty("oled", juce::var()).getProperty("mint", "#D8F3DC").toString());
            mouthDotColor = parseColor(colors.getProperty("mouth", juce::var()).getProperty("dot_color", "#3B4A52").toString());

            auto knobColors = colors.getProperty("knob", juce::var());
            knobGradientLight = parseColor(knobColors.getProperty("gradient_light", "#325555").toString());
            knobGradientDark = parseColor(knobColors.getProperty("gradient_dark", "#2c4949").toString());
            knobInsetLight = parseColor(knobColors.getProperty("inset_light", "#385f5f").toString());
            knobInsetDark = parseColor(knobColors.getProperty("inset_dark", "#263e3e").toString());

            auto ledColors = colors.getProperty("led", juce::var());
            ledFlow = parseColor(ledColors.getProperty("flow", "#42D697").toString());
            ledStruggle = parseColor(ledColors.getProperty("struggle", "#E8BF3D").toString());
            ledMeltdown = parseColor(ledColors.getProperty("meltdown", "#DB3F3F").toString());

            auto buttonColors = colors.getProperty("button", juce::var());
            buttonAutoOn = parseColor(buttonColors.getProperty("auto_on", "#42D697").toString());
            buttonDangerOn = parseColor(buttonColors.getProperty("danger_on", "#DB3F3F").toString());

            // Parse layout
            auto layout = root.getProperty("layout", juce::var());

            auto window = layout.getProperty("window", juce::var());
            windowBounds = {0, 0, (int)window.getProperty("width", 400), (int)window.getProperty("height", 600)};

            auto lcdPanel = layout.getProperty("lcd_panel", juce::var());
            lcdPanelBounds = {(int)lcdPanel.getProperty("x", 24), (int)lcdPanel.getProperty("y", 60),
                             (int)lcdPanel.getProperty("width", 352), (int)lcdPanel.getProperty("height", 150)};

            auto halftoneMouth = layout.getProperty("halftone_mouth", juce::var());
            halftoneMouthBounds = {(int)halftoneMouth.getProperty("x", 32), (int)halftoneMouth.getProperty("y", 68),
                                  (int)halftoneMouth.getProperty("width", 336), (int)halftoneMouth.getProperty("height", 134)};

            auto title = layout.getProperty("title", juce::var());
            titleBounds = {(int)title.getProperty("x", 0), (int)title.getProperty("y", 24),
                          (int)title.getProperty("width", 400), (int)title.getProperty("height", 20)};

            auto statusLED = layout.getProperty("status_led", juce::var());
            int ledDiameter = (int)statusLED.getProperty("diameter", 12);
            statusLEDBounds = {(int)statusLED.getProperty("x", 24), (int)statusLED.getProperty("y", 20),
                              ledDiameter, ledDiameter};

            auto ledLabel = layout.getProperty("led_label", juce::var());
            ledLabelBounds = {(int)ledLabel.getProperty("x", 44), (int)ledLabel.getProperty("y", 14),
                             (int)ledLabel.getProperty("width", 120), (int)ledLabel.getProperty("height", 24)};

            auto morphKnob = layout.getProperty("morph_knob", juce::var());
            int morphSize = (int)morphKnob.getProperty("size", 72);
            morphKnobBounds = {(int)morphKnob.getProperty("x", 90), (int)morphKnob.getProperty("y", 250),
                              morphSize, morphSize};

            auto intensityKnob = layout.getProperty("intensity_knob", juce::var());
            int intensitySize = (int)intensityKnob.getProperty("size", 72);
            intensityKnobBounds = {(int)intensityKnob.getProperty("x", 238), (int)intensityKnob.getProperty("y", 250),
                                  intensitySize, intensitySize};

            auto mixKnob = layout.getProperty("mix_knob", juce::var());
            int mixSize = (int)mixKnob.getProperty("size", 72);
            mixKnobBounds = {(int)mixKnob.getProperty("x", 164), (int)mixKnob.getProperty("y", 400),
                            mixSize, mixSize};

            auto autoButton = layout.getProperty("auto_button", juce::var());
            autoButtonBounds = {(int)autoButton.getProperty("x", 164), (int)autoButton.getProperty("y", 220),
                               (int)autoButton.getProperty("width", 72), (int)autoButton.getProperty("height", 22)};

            auto dangerButton = layout.getProperty("danger_button", juce::var());
            dangerButtonBounds = {(int)dangerButton.getProperty("x", 260), (int)dangerButton.getProperty("y", 220),
                                 (int)dangerButton.getProperty("width", 90), (int)dangerButton.getProperty("height", 22)};

            auto pairBadge = layout.getProperty("pair_badge", juce::var());
            pairBadgeBounds = {(int)pairBadge.getProperty("x", 164), (int)pairBadge.getProperty("y", 245),
                              (int)pairBadge.getProperty("width", 72), (int)pairBadge.getProperty("height", 14)};

            auto serialNumber = layout.getProperty("serial_number", juce::var());
            serialNumberBounds = {(int)serialNumber.getProperty("x", 0), (int)serialNumber.getProperty("y", 560),
                                 (int)serialNumber.getProperty("width", 400), (int)serialNumber.getProperty("height", 12)};

            // Parse timing
            auto timing = root.getProperty("timing", juce::var());
            mouthFPS = (int)timing.getProperty("mouth_fps", 10);
            knobFPS = (int)timing.getProperty("knob_fps", 60);
        }
    }
    else
    {
        // Fallback to hardcoded defaults if JSON not found
        chassisColor = juce::Colour(0xff2F4F4F);
        lcdBackground = juce::Colour(0xffF1F4F5);
        oledMint = juce::Colour(0xffD8F3DC);
        mouthDotColor = juce::Colour(0xff3B4A52);
        knobGradientLight = juce::Colour(0xff325555);
        knobGradientDark = juce::Colour(0xff2c4949);
        knobInsetLight = juce::Colour(0xff385f5f);
        knobInsetDark = juce::Colour(0xff263e3e);
        ledFlow = juce::Colour(0xff42D697);
        ledStruggle = juce::Colour(0xffE8BF3D);
        ledMeltdown = juce::Colour(0xffDB3F3F);
        buttonAutoOn = juce::Colour(0xff42D697);
        buttonDangerOn = juce::Colour(0xffDB3F3F);

        // Hardcoded layout bounds
        windowBounds = {0, 0, 400, 600};
        lcdPanelBounds = {24, 60, 352, 150};
        halftoneMouthBounds = {32, 68, 336, 134};
        titleBounds = {0, 24, 400, 20};
        statusLEDBounds = {24, 20, 12, 12};
        ledLabelBounds = {44, 14, 120, 24};
        morphKnobBounds = {90, 250, 72, 72};
        intensityKnobBounds = {238, 250, 72, 72};
        mixKnobBounds = {164, 400, 72, 72};
        autoButtonBounds = {164, 220, 72, 22};
        dangerButtonBounds = {260, 220, 90, 22};
        pairBadgeBounds = {164, 245, 72, 14};
        serialNumberBounds = {0, 560, 400, 12};
    }
}

juce::Colour DesignTokens::parseColor(const juce::String& hexString) const
{
    // Parse hex color string (e.g., "#2F4F4F")
    if (hexString.startsWith("#") && hexString.length() == 7)
    {
        int r = hexString.substring(1, 3).getHexValue32();
        int g = hexString.substring(3, 5).getHexValue32();
        int b = hexString.substring(5, 7).getHexValue32();
        return juce::Colour((juce::uint8)r, (juce::uint8)g, (juce::uint8)b);
    }
    return juce::Colours::white;
}

juce::Colour DesignTokens::getLEDColor(LEDState state) const
{
    switch (state)
    {
        case LEDState::FLOW:
            return ledFlow;
        case LEDState::STRUGGLE:
            return ledStruggle;
        case LEDState::MELTDOWN:
            return ledMeltdown;
        default:
            return ledFlow;
    }
}

juce::Colour DesignTokens::getButtonOnColor(bool isDanger) const
{
    return isDanger ? buttonDangerOn : buttonAutoOn;
}

juce::Font DesignTokens::getTitleFont() const
{
    return juce::Font(16.0f, juce::Font::bold);
}

juce::Font DesignTokens::getLabelFont() const
{
    return juce::Font(14.0f, juce::Font::bold);
}

juce::Font DesignTokens::getValueFont() const
{
    return juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain);
}

juce::Font DesignTokens::getStatusFont() const
{
    return juce::Font(11.0f, juce::Font::bold);
}

juce::Font DesignTokens::getSerialFont() const
{
    return juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain);
}
