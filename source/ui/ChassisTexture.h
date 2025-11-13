#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * ChassisTexture - Generates powder-coat texture for hardware aesthetic
 *
 * Purpose: Pre-render subtle grain texture (1200 dots, deterministic)
 * Performance: One-time generation in constructor, cached as Image
 */
class ChassisTexture
{
public:
    ChassisTexture();

    /**
     * Generate texture (call once in constructor or when size changes)
     * @param width Width in pixels
     * @param height Height in pixels
     */
    void generate(int width, int height);

    /**
     * Draw cached texture
     * @param g Graphics context
     * @param bounds Area to draw into
     */
    void draw(juce::Graphics& g, juce::Rectangle<int> bounds) const;

private:
    juce::Image cachedTexture;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChassisTexture)
};
