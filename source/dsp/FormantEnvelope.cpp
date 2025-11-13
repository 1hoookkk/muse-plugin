#include "FormantEnvelope.h"
#include <algorithm>
#include <cmath>

FormantEnvelope::FormantEnvelope()
{
}

void FormantEnvelope::generateEnvelope(std::vector<float>& envelope,
                                      const VowelShape& vowel,
                                      float intensity,
                                      float sampleRate,
                                      int fftSize)
{
    const int numBins = fftSize / 2 + 1;
    envelope.resize(numBins);

    // Map intensity to bandwidth scaling
    float bwScale = intensityToBandwidthScale(intensity);

    // Generate envelope by summing Gaussian peaks at each formant
    for (int bin = 0; bin < numBins; ++bin)
    {
        // Convert bin index to frequency
        float freq = (float)bin * sampleRate / (float)fftSize;

        // Sum contributions from all three formants
        float magnitude = 0.0f;
        magnitude += gaussianPeak(freq, vowel.f1, bwScale);
        magnitude += gaussianPeak(freq, vowel.f2, bwScale);
        magnitude += gaussianPeak(freq, vowel.f3, bwScale);

        // Add baseline (prevents complete silence between peaks)
        magnitude += 0.15f;

        // Clamp to reasonable range (prevent excessive boost)
        magnitude = std::min(magnitude, 2.5f);

        envelope[bin] = magnitude;
    }

    // Normalize envelope so peak magnitude is around 1.5-2.0
    // This allows formant peaks to boost by 3-6 dB
    float maxMag = *std::max_element(envelope.begin(), envelope.end());
    if (maxMag > 0.01f)
    {
        float normFactor = 1.8f / maxMag;
        for (auto& mag : envelope)
            mag *= normFactor;
    }
}

VowelShape FormantEnvelope::getAAVowel()
{
    // AA vowel ("cat") - from data/SHAPES_EXAMPLE.json
    return {
        { 700.0f, 90.0f, 0.0f },   // F1
        { 1220.0f, 110.0f, 0.0f }, // F2
        { 2600.0f, 170.0f, 0.0f }, // F3
        "AA"
    };
}

VowelShape FormantEnvelope::getVowelAA()
{
    return getAAVowel();
}

VowelShape FormantEnvelope::getVowelAH()
{
    // AH vowel ("father")
    return {
        { 730.0f, 90.0f, 0.0f },
        { 1090.0f, 110.0f, 0.0f },
        { 2440.0f, 170.0f, 0.0f },
        "AH"
    };
}

VowelShape FormantEnvelope::getVowelEE()
{
    // EE vowel ("beet")
    return {
        { 270.0f, 90.0f, 0.0f },
        { 2290.0f, 110.0f, 0.0f },
        { 3010.0f, 170.0f, 0.0f },
        "EE"
    };
}

VowelShape FormantEnvelope::getVowelOH()
{
    // OH vowel ("boat")
    return {
        { 570.0f, 90.0f, 0.0f },
        { 840.0f, 110.0f, 0.0f },
        { 2410.0f, 170.0f, 0.0f },
        "OH"
    };
}

VowelShape FormantEnvelope::getVowelOO()
{
    // OO vowel ("boot")
    return {
        { 300.0f, 90.0f, 0.0f },
        { 870.0f, 110.0f, 0.0f },
        { 2240.0f, 170.0f, 0.0f },
        "OO"
    };
}

float FormantEnvelope::intensityToBandwidthScale(float intensity)
{
    // M2: Fixed at intensity = 0.5 → scale = 1.0
    // M3 will implement full mapping:
    // intensity 0.0 → scale 2.0 (wide peaks)
    // intensity 0.5 → scale 1.0 (nominal)
    // intensity 1.0 → scale 0.5 (narrow peaks)

    // Linear interpolation for now
    return 2.0f - intensity * 1.5f;
}

float FormantEnvelope::gaussianPeak(float f, const Formant& formant, float bandwidthScale)
{
    // Gaussian peak centered at formant frequency
    // Peak height = 1.0 at center, decays with distance

    float scaledBandwidth = formant.bandwidth * bandwidthScale;

    // Avoid division by zero
    if (scaledBandwidth < 1.0f)
        scaledBandwidth = 1.0f;

    // Gaussian formula: exp(-((f - f0) / bw)^2)
    float delta = (f - formant.frequency) / scaledBandwidth;
    float gaussian = std::exp(-delta * delta);

    // Convert formant gain from dB to linear (currently all gains = 0 dB = 1.0×)
    float gainLinear = 1.0f; // std::pow(10.0f, formant.gain / 20.0f);

    return gaussian * gainLinear;
}
