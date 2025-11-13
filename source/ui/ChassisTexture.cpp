#include "ChassisTexture.h"

ChassisTexture::ChassisTexture()
{
}

void ChassisTexture::generate(int width, int height)
{
    // Create image for texture (ARGB format)
    cachedTexture = juce::Image(juce::Image::ARGB, width, height, true);

    juce::Graphics g(cachedTexture);

    // Clear to transparent
    g.fillAll(juce::Colours::transparentBlack);

    // Generate 1200 random white dots with low alpha (powder-coat grain)
    // Use deterministic seed for consistency across sessions
    juce::Random random(42);

    for (int i = 0; i < 1200; ++i)
    {
        float x = random.nextFloat() * width;
        float y = random.nextFloat() * height;
        float alpha = random.nextFloat() * 0.04f;  // Very subtle (0-4% alpha)

        g.setColour(juce::Colours::white.withAlpha(alpha));
        g.fillRect(x, y, 1.0f, 1.0f);
    }
}

void ChassisTexture::draw(juce::Graphics& g, juce::Rectangle<int> bounds) const
{
    if (cachedTexture.isValid())
    {
        g.drawImageAt(cachedTexture, bounds.getX(), bounds.getY());
    }
}
