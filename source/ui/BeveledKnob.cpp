#include "BeveledKnob.h"

BeveledKnob::BeveledKnob(juce::AudioProcessorValueTreeState& apvts_,
                         const juce::String& parameterID_,
                         const juce::String& labelText_)
    : apvts(apvts_), parameterID(parameterID_), labelText(labelText_)
{
    setSize(72, 72);
}

float BeveledKnob::getValue() const
{
    return apvts.getRawParameterValue(parameterID)->load();
}

void BeveledKnob::setValue(float newValue)
{
    // Clamp to [0, 1]
    newValue = juce::jlimit(0.0f, 1.0f, newValue);

    // Update parameter via APVTS
    if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameterID)))
    {
        param->setValueNotifyingHost(newValue);
    }

    repaint();
}

juce::String BeveledKnob::getValueText() const
{
    float value = getValue();
    return juce::String(value, 1);  // 1 decimal place
}

void BeveledKnob::paint(juce::Graphics& g)
{
    auto& tokens = DesignTokens::getInstance();
    auto bounds = getLocalBounds().toFloat();
    auto center = bounds.getCentre();
    float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    float radius = diameter * 0.5f;

    // Drop shadow (1px offset, black alpha=0.5)
    juce::Path shadowPath;
    shadowPath.addEllipse(bounds.translated(1.0f, 1.0f));
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillPath(shadowPath);

    // Outer bezel (radial gradient)
    juce::ColourGradient gradient(
        tokens.getKnobGradientLight(), center.x, center.y - radius * 0.5f,
        tokens.getKnobGradientDark(), center.x, center.y + radius * 0.5f,
        true
    );
    g.setGradientFill(gradient);
    g.fillEllipse(bounds);

    // Inset shadows (3D beveled effect)
    // Top-left highlight
    g.setColour(tokens.getKnobInsetLight().withAlpha(0.4f));
    juce::Path highlightPath;
    highlightPath.addCentredArc(center.x, center.y, radius * 0.95f, radius * 0.95f,
                                0.0f, juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 1.75f, true);
    g.strokePath(highlightPath, juce::PathStrokeType(2.0f));

    // Bottom-right shadow
    g.setColour(tokens.getKnobInsetDark().withAlpha(0.6f));
    juce::Path shadowInsetPath;
    shadowInsetPath.addCentredArc(center.x, center.y, radius * 0.95f, radius * 0.95f,
                                  0.0f, juce::MathConstants<float>::pi * 0.25f,
                                  juce::MathConstants<float>::pi * 0.75f, true);
    g.strokePath(shadowInsetPath, juce::PathStrokeType(2.0f));

    // Center circle (chassis color, 80% of outer diameter)
    float innerDiameter = diameter * 0.8f;
    auto innerBounds = bounds.withSizeKeepingCentre(innerDiameter, innerDiameter);
    g.setColour(tokens.getChassisColor());
    g.fillEllipse(innerBounds);

    // Mint indicator line (rotates with value)
    float value = getValue();
    float angle = juce::MathConstants<float>::pi * 1.25f +
                  value * juce::MathConstants<float>::pi * 1.5f;  // 270° rotation range

    float indicatorLength = 12.0f;
    float indicatorStart = 6.0f;  // 6px from center

    juce::Point<float> lineStart(
        center.x + std::cos(angle) * indicatorStart,
        center.y + std::sin(angle) * indicatorStart
    );
    juce::Point<float> lineEnd(
        center.x + std::cos(angle) * (indicatorStart + indicatorLength),
        center.y + std::sin(angle) * (indicatorStart + indicatorLength)
    );

    g.setColour(tokens.getOLEDMint());
    g.drawLine(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y, 2.0f);
}

void BeveledKnob::mouseDown(const juce::MouseEvent& event)
{
    dragStartValue = getValue();
    dragStartY = event.y;
}

void BeveledKnob::mouseDrag(const juce::MouseEvent& event)
{
    // Vertical drag: 150px travel = full range
    int dragDelta = dragStartY - event.y;  // Up = positive
    float valueDelta = dragDelta / 150.0f;

    float newValue = dragStartValue + valueDelta;
    setValue(newValue);
}

void BeveledKnob::mouseDoubleClick(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    setValue(0.5f);  // Reset to center
}

void BeveledKnob::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    juce::ignoreUnused(event);

    // Scroll wheel: increment/decrement by 0.01
    float delta = wheel.deltaY * 0.05f;
    float newValue = getValue() + delta;
    setValue(newValue);
}

void BeveledKnob::drawOLEDText(juce::Graphics& g, const juce::String& text,
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
