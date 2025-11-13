#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "DesignTokens.h"

/**
 * HalftoneMouth - Dense dot matrix visualization (10 FPS)
 *
 * Purpose: "Haunted hardware" mouth animation with intentional stutter
 * Features:
 * - Dense dot grid (100-140 cols × 50-60 rows)
 * - Almond lip shape (ellipse with horizontal tapers)
 * - Edge-weighted dot sizes (larger at edges for depth)
 * - 10 FPS update cadence (NOT 60 FPS - this is the design!)
 * - Brightness scales with audio RMS
 * - Shape morphs with morph/intensity parameters
 * - Breathing animation (~2% scale, slow)
 * - Transient pulse (scale increase on peaks)
 */
class HalftoneMouth : public juce::Component
{
public:
    HalftoneMouth();

    /**
     * Update mouth state (called at 10 FPS)
     * @param rmsLevel Audio RMS level (0.0 - 1.0)
     * @param morph Morph parameter (0.0 - 1.0)
     * @param intensity Intensity parameter (0.0 - 1.0)
     */
    void updateState(float rmsLevel, float morph, float intensity);

    /**
     * Trigger update (call this every 6 frames from 60 FPS timer)
     */
    void triggerUpdate();

    void paint(juce::Graphics& g) override;

private:
    struct DotState
    {
        float x;
        float y;
        float size;
        float brightness;
        bool visible;
    };

    void regenerateDots();
    float calculateDotSize(float x, float y, float normalizedX, float normalizedY) const;
    bool isInsideAlmondShape(float x, float y, float width, float height) const;

    // Current state (updated at 10 FPS)
    float currentRMS = 0.0f;
    float currentMorph = 0.5f;
    float currentIntensity = 0.5f;

    // Animation state
    float breathingPhase = 0.0f;
    float pulseScale = 1.0f;
    int framesSinceLastPeak = 0;

    // Dot grid configuration
    static constexpr int COLS = 120;
    static constexpr int ROWS = 55;
    std::vector<DotState> dots;

    // Pre-computed for performance
    bool dotsGenerated = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HalftoneMouth)
};
