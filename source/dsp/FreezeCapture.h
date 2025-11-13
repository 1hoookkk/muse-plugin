#pragma once

#include <vector>
#include <algorithm>

/**
 * FreezeCapture - Manages spectral freeze capture, hold, and crossfade
 *
 * Behavior:
 * - OFF→ON: Capture current spectrum, crossfade in over ~50ms
 * - ON (holding): Use frozen spectrum, ignore live input
 * - ON→OFF: Crossfade back to live spectrum over ~100ms
 */
class FreezeCapture
{
public:
    FreezeCapture();

    /**
     * Prepare for processing
     * @param numBins Number of FFT bins (FFT_SIZE/2 + 1)
     * @param sampleRate Sample rate in Hz
     * @param hopSize STFT hop size in samples
     */
    void prepare(int numBins, double sampleRate, int hopSize);

    /**
     * Reset state
     */
    void reset();

    /**
     * Update freeze state and process spectrum
     * @param liveMagnitudes Current input spectrum
     * @param outputMagnitudes Output spectrum (frozen, live, or crossfaded)
     * @param numBins Number of bins
     * @param freezeButton Freeze button state (true = freeze ON)
     */
    void processSpectrum(const float* liveMagnitudes,
                        float* outputMagnitudes,
                        int numBins,
                        bool freezeButton);

    /**
     * Check if freeze is active
     */
    bool isFreezeActive() const { return freezeActive; }

    /**
     * Get current crossfade position (0.0 = fully frozen, 1.0 = fully live)
     */
    float getCrossfadePosition() const { return crossfadeRamp; }

private:
    // Frozen spectrum buffer
    std::vector<float> frozenSpectrum;

    // State flags
    bool freezeActive = false;
    bool previousFreezeButton = false;

    // Crossfade state
    float crossfadeRamp = 1.0f;  // 0.0 = frozen, 1.0 = live
    float crossfadeRate = 0.0f;  // Change per frame

    // Configuration
    int numBins_ = 0;
    double sampleRate_ = 48000.0;
    int hopSize_ = 512;

    // Crossfade timing (in frames)
    static constexpr float ONSET_TIME_MS = 50.0f;   // 50ms onset
    static constexpr float RELEASE_TIME_MS = 100.0f; // 100ms release
};
