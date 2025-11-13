#include "FreezeCapture.h"
#include <cmath>

FreezeCapture::FreezeCapture()
{
}

void FreezeCapture::prepare(int numBins, double sampleRate, int hopSize)
{
    numBins_ = numBins;
    sampleRate_ = sampleRate;
    hopSize_ = hopSize;

    // Allocate frozen spectrum buffer
    frozenSpectrum.resize(numBins, 0.0f);

    reset();
}

void FreezeCapture::reset()
{
    std::fill(frozenSpectrum.begin(), frozenSpectrum.end(), 0.0f);
    freezeActive = false;
    previousFreezeButton = false;
    crossfadeRamp = 1.0f;  // Start in live mode
    crossfadeRate = 0.0f;
}

void FreezeCapture::processSpectrum(const float* liveMagnitudes,
                                   float* outputMagnitudes,
                                   int numBins,
                                   bool freezeButton)
{
    // Detect freeze button transitions
    bool buttonPressed = freezeButton && !previousFreezeButton;
    bool buttonReleased = !freezeButton && previousFreezeButton;
    previousFreezeButton = freezeButton;

    // Handle freeze button state changes
    if (buttonPressed)
    {
        // OFF→ON: Capture current spectrum
        for (int i = 0; i < numBins; ++i)
        {
            frozenSpectrum[i] = liveMagnitudes[i];
        }

        freezeActive = true;

        // Start crossfade to frozen (50ms onset)
        float framesPerSecond = sampleRate_ / (float)hopSize_;
        float onsetFrames = (ONSET_TIME_MS / 1000.0f) * framesPerSecond;
        crossfadeRate = -1.0f / std::max(1.0f, onsetFrames);  // Negative = towards frozen
        crossfadeRamp = 1.0f;  // Start from live
    }
    else if (buttonReleased)
    {
        // ON→OFF: Release back to live
        freezeActive = false;

        // Start crossfade to live (100ms release)
        float framesPerSecond = sampleRate_ / (float)hopSize_;
        float releaseFrames = (RELEASE_TIME_MS / 1000.0f) * framesPerSecond;
        crossfadeRate = 1.0f / std::max(1.0f, releaseFrames);  // Positive = towards live
        // crossfadeRamp stays at current position
    }

    // Update crossfade ramp
    if (crossfadeRate != 0.0f)
    {
        crossfadeRamp += crossfadeRate;

        // Clamp to [0, 1]
        if (crossfadeRamp <= 0.0f)
        {
            crossfadeRamp = 0.0f;
            crossfadeRate = 0.0f;  // Finished fading to frozen
        }
        else if (crossfadeRamp >= 1.0f)
        {
            crossfadeRamp = 1.0f;
            crossfadeRate = 0.0f;  // Finished fading to live
        }
    }

    // Output spectrum based on crossfade state
    if (crossfadeRamp >= 0.999f)
    {
        // Fully live - just pass through
        for (int i = 0; i < numBins; ++i)
        {
            outputMagnitudes[i] = liveMagnitudes[i];
        }
    }
    else if (crossfadeRamp <= 0.001f)
    {
        // Fully frozen - use frozen spectrum
        for (int i = 0; i < numBins; ++i)
        {
            outputMagnitudes[i] = frozenSpectrum[i];
        }
    }
    else
    {
        // Crossfading - linear interpolation
        float frozenGain = 1.0f - crossfadeRamp;
        float liveGain = crossfadeRamp;

        for (int i = 0; i < numBins; ++i)
        {
            outputMagnitudes[i] = frozenSpectrum[i] * frozenGain +
                                 liveMagnitudes[i] * liveGain;
        }
    }
}
