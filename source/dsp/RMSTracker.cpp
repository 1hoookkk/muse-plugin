#include "RMSTracker.h"

RMSTracker::RMSTracker()
{
}

void RMSTracker::prepare(double sampleRate, int hopSize)
{
    sampleRate_ = sampleRate;
    hopSize_ = hopSize;

    // Calculate smoothing coefficient for ~100ms time constant
    // Time constant formula: coeff = exp(-1 / (time_constant * frames_per_second))
    // frames_per_second = sampleRate / hopSize
    float framesPerSecond = static_cast<float>(sampleRate / hopSize);
    float timeConstant = 0.1f;  // 100ms
    smoothingCoeff = std::exp(-1.0f / (timeConstant * framesPerSecond));

    // Clamp to reasonable range
    smoothingCoeff = std::max(0.9f, std::min(0.99f, smoothingCoeff));

    reset();
}

void RMSTracker::reset()
{
    smoothedRMS = 0.0f;
}

float RMSTracker::computeRMS(const float* buffer, int numSamples)
{
    if (numSamples <= 0)
        return smoothedRMS;

    // Compute instantaneous RMS
    float sumSquares = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = buffer[i];

        // Sanitize input
        if (std::isnan(sample) || std::isinf(sample))
            sample = 0.0f;

        sumSquares += sample * sample;
    }

    float instantRMS = std::sqrt(sumSquares / numSamples);

    // Sanitize RMS
    if (std::isnan(instantRMS) || std::isinf(instantRMS))
        instantRMS = 0.0f;

    // Exponential smoothing
    smoothedRMS = smoothedRMS * smoothingCoeff + instantRMS * (1.0f - smoothingCoeff);

    return smoothedRMS;
}
