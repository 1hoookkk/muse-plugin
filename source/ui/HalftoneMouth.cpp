#include "HalftoneMouth.h"

HalftoneMouth::HalftoneMouth()
{
    // Pre-allocate dot vector
    dots.reserve(COLS * ROWS);
}

void HalftoneMouth::updateState(float rmsLevel, float morph, float intensity)
{
    // Store current state
    float prevRMS = currentRMS;
    currentRMS = rmsLevel;
    currentMorph = morph;
    currentIntensity = intensity;

    // Breathing animation (slow sine wave, ~2% amplitude)
    breathingPhase += 0.02f;  // ~0.3 Hz at 10 FPS
    if (breathingPhase > juce::MathConstants<float>::twoPi)
        breathingPhase -= juce::MathConstants<float>::twoPi;

    // Transient pulse detection (RMS spike)
    float rmsDelta = currentRMS - prevRMS;
    if (rmsDelta > 0.1f && framesSinceLastPeak > 3)  // Spike > 0.1, not too frequent
    {
        pulseScale = 1.15f;  // +15% scale
        framesSinceLastPeak = 0;
    }

    // Decay pulse smoothly
    pulseScale = pulseScale * 0.8f + 1.0f * 0.2f;  // Smooth back to 1.0
    ++framesSinceLastPeak;

    // Trigger repaint (10 FPS)
    repaint();
}

void HalftoneMouth::triggerUpdate()
{
    // This is called from editor's 60 FPS timer, every 6 frames
    // The actual state update happens when updateState() is called
    // This method exists for API clarity
}

void HalftoneMouth::paint(juce::Graphics& g)
{
    auto& tokens = DesignTokens::getInstance();
    auto bounds = getLocalBounds().toFloat();

    // Regenerate dots if not yet generated or bounds changed
    if (!dotsGenerated || dots.empty())
    {
        regenerateDots();
        dotsGenerated = true;
    }

    // Calculate overall scale (breathing + pulse)
    float breathingScale = 1.0f + std::sin(breathingPhase) * 0.02f;  // ±2%
    float totalScale = breathingScale * pulseScale;

    // Calculate mouth shape parameters (morph affects width/height ratio)
    float baseWidth = bounds.getWidth() * 0.48f;   // 48% of display width
    float baseHeight = bounds.getHeight() * 0.33f; // 33% of display height

    // Morph affects mouth shape: 0.0 = wider/shorter, 1.0 = narrower/taller
    float widthScale = 1.0f + (0.5f - currentMorph) * 0.3f;  // 0.85 - 1.15
    float heightScale = 1.0f + (currentMorph - 0.5f) * 0.4f; // 0.8 - 1.2

    float mouthWidth = baseWidth * widthScale * totalScale;
    float mouthHeight = baseHeight * heightScale * totalScale;

    // Center of mouth
    juce::Point<float> center = bounds.getCentre();

    // Draw dots
    g.setColour(tokens.getMouthDotColor());

    for (const auto& dot : dots)
    {
        if (!dot.visible)
            continue;

        // Transform dot position (scale from center)
        float scaledX = center.x + (dot.x - center.x) * totalScale;
        float scaledY = center.y + (dot.y - center.y) * totalScale;

        // Check if scaled position is inside almond shape
        float normX = (scaledX - center.x) / mouthWidth;
        float normY = (scaledY - center.y) / mouthHeight;

        // Almond shape test (ellipse with horizontal tapers)
        float distance = normX * normX + normY * normY;
        if (distance > 1.0f)
            continue;

        // Brightness modulation (RMS affects opacity)
        float brightness = 0.3f + currentRMS * 0.7f;  // 30-100% opacity
        brightness = juce::jlimit(0.0f, 1.0f, brightness);

        // Intensity affects contrast (higher = more visible dots)
        float contrastBoost = 1.0f + currentIntensity * 0.5f;
        brightness *= contrastBoost;

        // Apply brightness to dot color
        g.setColour(tokens.getMouthDotColor().withAlpha(brightness));

        // Draw dot (size already calculated during generation)
        float dotSize = dot.size;
        g.fillEllipse(scaledX - dotSize * 0.5f, scaledY - dotSize * 0.5f,
                      dotSize, dotSize);
    }
}

void HalftoneMouth::regenerateDots()
{
    dots.clear();
    auto bounds = getLocalBounds().toFloat();

    if (bounds.isEmpty())
        return;

    // Calculate grid spacing
    float colSpacing = bounds.getWidth() / (COLS - 1);
    float rowSpacing = bounds.getHeight() / (ROWS - 1);

    // Generate dots
    for (int row = 0; row < ROWS; ++row)
    {
        for (int col = 0; col < COLS; ++col)
        {
            float x = bounds.getX() + col * colSpacing;
            float y = bounds.getY() + row * rowSpacing;

            // Normalized position [0, 1]
            float normX = static_cast<float>(col) / (COLS - 1);
            float normY = static_cast<float>(row) / (ROWS - 1);

            // Calculate dot size (edge-weighted: larger at edges)
            float dotSize = calculateDotSize(x, y, normX, normY);

            // Check if inside almond shape (use base shape for generation)
            bool visible = isInsideAlmondShape(x, y, bounds.getWidth(), bounds.getHeight());

            dots.push_back({x, y, dotSize, 1.0f, visible});
        }
    }
}

float HalftoneMouth::calculateDotSize(float x, float y, float normalizedX, float normalizedY) const
{
    juce::ignoreUnused(x, y);

    // Distance from center (0.0 = center, 1.0 = edge)
    float centerX = 0.5f;
    float centerY = 0.5f;
    float dx = normalizedX - centerX;
    float dy = normalizedY - centerY;
    float distanceFromCenter = std::sqrt(dx * dx + dy * dy);

    // Edge-weighted: larger dots at edges (1.0-2.0 px at center, 2.0-3.5 px at edges)
    float baseDotSize = 1.5f;
    float edgeBoost = distanceFromCenter * 1.5f;  // 0 - 1.5× boost
    float dotSize = baseDotSize + edgeBoost;

    return juce::jlimit(1.0f, 3.5f, dotSize);
}

bool HalftoneMouth::isInsideAlmondShape(float x, float y, float width, float height) const
{
    // Almond shape: ellipse with horizontal tapers
    // Center of bounds
    float centerX = width * 0.5f;
    float centerY = height * 0.5f;

    // Mouth dimensions (48% width, 33% height)
    float mouthWidth = width * 0.48f;
    float mouthHeight = height * 0.33f;

    // Normalized position relative to mouth center
    float normX = (x - centerX) / mouthWidth;
    float normY = (y - centerY) / mouthHeight;

    // Ellipse test
    float distance = normX * normX + normY * normY;

    // Horizontal taper (narrower at left/right edges)
    float taperFactor = 1.0f - std::abs(normX) * 0.2f;  // Reduce height by 20% at edges
    distance = normX * normX + (normY / taperFactor) * (normY / taperFactor);

    return distance <= 1.0f;
}
