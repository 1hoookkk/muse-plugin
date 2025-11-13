#pragma once

#include <cmath>
#include <algorithm>

/**
 * RMSTracker - Real-time RMS (Root Mean Square) measurement with smoothing
 *
 * Purpose: Track audio signal level for adaptive gain compensation
 * Features:
 * - Per-block RMS computation
 * - Exponential smoothing (100-200ms time constant)
 * - RT-safe (no allocations, bounded computation)
 */
class RMSTracker
{
public:
    RMSTracker();

    /**
     * Prepare for processing
     * @param sampleRate Sample rate in Hz
     * @param hopSize Processing hop size (for smoothing coefficient calculation)
     */
    void prepare(double sampleRate, int hopSize);

    /**
     * Reset smoothed RMS to zero
     */
    void reset();

    /**
     * Compute RMS over block and update smoothed value
     * @param buffer Input audio buffer
     * @param numSamples Number of samples in buffer
     * @return Current smoothed RMS level
     */
    float computeRMS(const float* buffer, int numSamples);

    /**
     * Get current smoothed RMS level
     */
    float getSmoothedRMS() const { return smoothedRMS; }

private:
    float smoothedRMS = 0.0f;
    float smoothingCoeff = 0.98f;  // ~100ms time constant
    double sampleRate_ = 48000.0;
    int hopSize_ = 512;
};
