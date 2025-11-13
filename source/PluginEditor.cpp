#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      morphKnob(p.getAPVTS(), "morph", "MORPH"),
      intensityKnob(p.getAPVTS(), "intensity", "INTENSITY"),
      mixKnob(p.getAPVTS(), "mix", "MIX"),
      freezeButton(p.getAPVTS(), "auto", "FREEZE", false),
      dangerButton(p.getAPVTS(), "danger", "DANGER", true)
{
    juce::ignoreUnused (processorRef);

    // Generate chassis texture
    chassisTexture.generate(400, 600);

    // Add UI components
    addAndMakeVisible(morphKnob);
    addAndMakeVisible(intensityKnob);
    addAndMakeVisible(mixKnob);
    addAndMakeVisible(freezeButton);
    addAndMakeVisible(dangerButton);
    addAndMakeVisible(statusLED);

    // Inspector (dev tool)
    addAndMakeVisible (inspectButton);

    // this chunk of code instantiates and opens the melatonin inspector
    inspectButton.onClick = [&] {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }

        inspector->setVisible (true);
    };

    // M5: Set window size to 400×600px (fixed, as per UI spec)
    setSize (400, 600);
    setResizable (false, false);

    // Start 60 FPS timer for smooth knob updates
    startTimerHz(60);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

void PluginEditor::timerCallback()
{
    // Update status LED based on RMS level from processor
    float rmsLevel = processorRef.getCurrentRMSLevel();
    statusLED.updateLevel(rmsLevel);

    // Increment frame counter for 10 FPS mouth updates (M6)
    ++frameCounter;

    // M6: Every 6 frames (10 FPS), update mouth
    // if (frameCounter % 6 == 0) {
    //     // TODO: Update HalftoneMouth component
    // }
}

void PluginEditor::paint (juce::Graphics& g)
{
    auto& tokens = DesignTokens::getInstance();

    // Fill chassis background
    g.fillAll(tokens.getChassisColor());

    // Draw powder-coat texture
    chassisTexture.draw(g, getLocalBounds());

    // Draw LCD panel
    drawLCDPanel(g);

    // Draw title "MUSE"
    drawTitle(g);

    // Draw pair badge
    drawPairBadge(g);

    // Draw serial number
    drawSerialNumber(g);

    // Draw knob labels and values
    drawKnobLabels(g);

    // Draw LED label
    drawLEDLabel(g);
}

void PluginEditor::drawLCDPanel(juce::Graphics& g)
{
    auto& tokens = DesignTokens::getInstance();
    auto bounds = tokens.getLCDPanelBounds().toFloat();

    // Fill with LCD background
    g.setColour(tokens.getLCDBackground());
    g.fillRect(bounds);

    // Draw beveled border (3D inset effect)
    // Outer highlight (top-left)
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawLine(bounds.getX(), bounds.getY(),
               bounds.getRight(), bounds.getY(), 1.0f);
    g.drawLine(bounds.getX(), bounds.getY(),
               bounds.getX(), bounds.getBottom(), 1.0f);

    // Inner shadow (bottom-right)
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawLine(bounds.getX(), bounds.getBottom(),
               bounds.getRight(), bounds.getBottom(), 1.0f);
    g.drawLine(bounds.getRight(), bounds.getY(),
               bounds.getRight(), bounds.getBottom(), 1.0f);

    // Deep inner shadow (glass depth)
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    auto innerBounds = bounds.reduced(1.0f);
    g.drawLine(innerBounds.getX(), innerBounds.getBottom(),
               innerBounds.getRight(), innerBounds.getBottom(), 1.0f);
    g.drawLine(innerBounds.getRight(), innerBounds.getY(),
               innerBounds.getRight(), innerBounds.getBottom(), 1.0f);

    // M6: HalftoneMouth will be drawn here
}

void PluginEditor::drawTitle(juce::Graphics& g)
{
    auto& tokens = DesignTokens::getInstance();
    auto bounds = tokens.getTitleBounds().toFloat();

    g.setFont(tokens.getTitleFont());
    drawOLEDText(g, "MUSE", bounds, juce::Justification::centred);
}

void PluginEditor::drawPairBadge(juce::Graphics& g)
{
    auto& tokens = DesignTokens::getInstance();
    auto bounds = tokens.getPairBadgeBounds().toFloat();

    // Read current pair parameter
    int pair = static_cast<int>(processorRef.getAPVTS().getRawParameterValue("pair")->load());

    juce::String pairName;
    switch (pair)
    {
        case 0: pairName = "VOWEL"; break;
        case 1: pairName = "BELL"; break;
        case 2: pairName = "LOW"; break;
        case 3: pairName = "SUB"; break;
        default: pairName = "VOWEL"; break;
    }

    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.setColour(tokens.getOLEDMint().withAlpha(0.7f));
    g.drawText(pairName, bounds, juce::Justification::centred, false);
}

void PluginEditor::drawSerialNumber(juce::Graphics& g)
{
    auto& tokens = DesignTokens::getInstance();
    auto bounds = tokens.getSerialNumberBounds().toFloat();

    g.setFont(tokens.getSerialFont());
    g.setColour(tokens.getOLEDMint().withAlpha(0.15f));
    g.drawText("EMU-Z-1993-MUSE", bounds, juce::Justification::centred, false);
}

void PluginEditor::drawKnobLabels(juce::Graphics& g)
{
    auto& tokens = DesignTokens::getInstance();
    g.setFont(tokens.getLabelFont());

    // MORPH knob
    auto morphBounds = tokens.getMorphKnobBounds().toFloat();
    auto morphLabelBounds = morphBounds.withY(morphBounds.getY() - 20.0f).withHeight(16.0f);
    drawOLEDText(g, "MORPH", morphLabelBounds, juce::Justification::centred);

    auto morphValueBounds = morphBounds.withY(morphBounds.getBottom() + 4.0f).withHeight(14.0f);
    g.setFont(tokens.getValueFont());
    drawOLEDText(g, morphKnob.getValueText(), morphValueBounds, juce::Justification::centred);

    // INTENSITY knob
    g.setFont(tokens.getLabelFont());
    auto intensityBounds = tokens.getIntensityKnobBounds().toFloat();
    auto intensityLabelBounds = intensityBounds.withY(intensityBounds.getY() - 20.0f).withHeight(16.0f);
    drawOLEDText(g, "INTENSITY", intensityLabelBounds, juce::Justification::centred);

    auto intensityValueBounds = intensityBounds.withY(intensityBounds.getBottom() + 4.0f).withHeight(14.0f);
    g.setFont(tokens.getValueFont());
    drawOLEDText(g, intensityKnob.getValueText(), intensityValueBounds, juce::Justification::centred);

    // MIX knob
    g.setFont(tokens.getLabelFont());
    auto mixBounds = tokens.getMixKnobBounds().toFloat();
    auto mixLabelBounds = mixBounds.withY(mixBounds.getY() - 20.0f).withHeight(16.0f);
    drawOLEDText(g, "MIX", mixLabelBounds, juce::Justification::centred);

    auto mixValueBounds = mixBounds.withY(mixBounds.getBottom() + 4.0f).withHeight(14.0f);
    g.setFont(tokens.getValueFont());
    drawOLEDText(g, mixKnob.getValueText(), mixValueBounds, juce::Justification::centred);
}

void PluginEditor::drawLEDLabel(juce::Graphics& g)
{
    auto& tokens = DesignTokens::getInstance();
    auto bounds = tokens.getLEDLabelBounds().toFloat();

    g.setFont(tokens.getStatusFont());
    g.setColour(tokens.getOLEDMint().withAlpha(0.85f));
    g.drawText(statusLED.getStateName(), bounds, juce::Justification::centredLeft, false);
}

void PluginEditor::drawOLEDText(juce::Graphics& g, const juce::String& text,
                                juce::Rectangle<float> bounds,
                                juce::Justification justification)
{
    auto& tokens = DesignTokens::getInstance();
    juce::Colour mint = tokens.getOLEDMint();

    // Outer glow (8px radius)
    g.setColour(mint.withAlpha(0.15f));
    for (int dx = -2; dx <= 2; dx += 4)
    {
        for (int dy = -2; dy <= 2; dy += 4)
        {
            if (dx == 0 && dy == 0) continue;
            g.drawText(text, bounds.translated((float)dx, (float)dy), justification, false);
        }
    }

    // Middle glow (4px)
    g.setColour(mint.withAlpha(0.25f));
    for (int dx = -1; dx <= 1; dx += 2)
    {
        for (int dy = -1; dy <= 1; dy += 2)
        {
            g.drawText(text, bounds.translated((float)dx, (float)dy), justification, false);
        }
    }

    // Inner glow (2px)
    g.setColour(mint.withAlpha(0.35f));
    g.drawText(text, bounds.translated(0.0f, -1.0f), justification, false);
    g.drawText(text, bounds.translated(0.0f, 1.0f), justification, false);
    g.drawText(text, bounds.translated(-1.0f, 0.0f), justification, false);
    g.drawText(text, bounds.translated(1.0f, 0.0f), justification, false);

    // Core text
    g.setColour(mint);
    g.drawText(text, bounds, justification, false);
}

void PluginEditor::resized()
{
    auto& tokens = DesignTokens::getInstance();

    // Position all components using design tokens
    morphKnob.setBounds(tokens.getMorphKnobBounds());
    intensityKnob.setBounds(tokens.getIntensityKnobBounds());
    mixKnob.setBounds(tokens.getMixKnobBounds());
    freezeButton.setBounds(tokens.getAutoButtonBounds());
    dangerButton.setBounds(tokens.getDangerButtonBounds());
    statusLED.setBounds(tokens.getStatusLEDBounds());

    // Inspector button (bottom right, dev tool)
    auto area = getLocalBounds();
    inspectButton.setBounds(area.removeFromBottom(60).reduced(150, 10));
}
