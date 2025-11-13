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

VowelShape FormantEnvelope::getVowelForPair(int pair, float morph)
{
    // Clamp morph to [0, 1]
    morph = std::max(0.0f, std::min(1.0f, morph));

    switch (pair)
    {
        case 0:  // VOWEL: AA → AH → EE (3-stage)
        {
            if (morph < 0.5f)
            {
                // First half: AA → AH
                float t = morph * 2.0f;  // Remap [0, 0.5] to [0, 1]
                return morphVowels(getVowelAA(), getVowelAH(), t);
            }
            else
            {
                // Second half: AH → EE
                float t = (morph - 0.5f) * 2.0f;  // Remap [0.5, 1] to [0, 1]
                return morphVowels(getVowelAH(), getVowelEE(), t);
            }
        }

        case 1:  // BELL: OH → OO (2-stage)
            return morphVowels(getVowelOH(), getVowelOO(), morph);

        case 2:  // LOW: AA → OO (2-stage)
            return morphVowels(getVowelAA(), getVowelOO(), morph);

        case 3:  // SUB: AH (static, morph ignored)
        default:
            return getVowelAH();
    }
}

VowelShape FormantEnvelope::morphVowels(const VowelShape& vowel1, const VowelShape& vowel2, float morph)
{
    // Clamp morph to [0, 1]
    morph = std::max(0.0f, std::min(1.0f, morph));

    VowelShape result;

    // Linearly interpolate formant frequencies and bandwidths
    result.f1.frequency = vowel1.f1.frequency * (1.0f - morph) + vowel2.f1.frequency * morph;
    result.f1.bandwidth = vowel1.f1.bandwidth * (1.0f - morph) + vowel2.f1.bandwidth * morph;
    result.f1.gain = vowel1.f1.gain * (1.0f - morph) + vowel2.f1.gain * morph;

    result.f2.frequency = vowel1.f2.frequency * (1.0f - morph) + vowel2.f2.frequency * morph;
    result.f2.bandwidth = vowel1.f2.bandwidth * (1.0f - morph) + vowel2.f2.bandwidth * morph;
    result.f2.gain = vowel1.f2.gain * (1.0f - morph) + vowel2.f2.gain * morph;

    result.f3.frequency = vowel1.f3.frequency * (1.0f - morph) + vowel2.f3.frequency * morph;
    result.f3.bandwidth = vowel1.f3.bandwidth * (1.0f - morph) + vowel2.f3.bandwidth * morph;
    result.f3.gain = vowel1.f3.gain * (1.0f - morph) + vowel2.f3.gain * morph;

    result.name = "Morphed";

    return result;
}

float FormantEnvelope::intensityToBandwidthScale(float intensity)
{
    // M3: Full intensity mapping
    // intensity 0.0 → scale 2.5 (very wide peaks, subtle shaping)
    // intensity 0.5 → scale 1.0 (nominal bandwidth)
    // intensity 1.0 → scale 0.4 (very narrow, aggressive shaping)

    // Linear interpolation
    float scale = 2.5f - intensity * 2.1f;

    // Clamp to safe range
    return std::max(0.4f, std::min(2.5f, scale));
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
