#pragma once

#include <vector>
#include <cmath>

/**
 * Formant data structure representing a resonant peak
 */
struct Formant
{
    float frequency;  // Hz
    float bandwidth;  // Hz
    float gain;       // dB (typically 0.0)
};

/**
 * Vowel shape containing 3 formant peaks
 */
struct VowelShape
{
    Formant f1, f2, f3;
    const char* name;
};

/**
 * FormantEnvelope - Generates spectral envelopes with vowel-like formant peaks
 *
 * M2: Hardcoded AA vowel only
 * M3: Will add morphing between vowels
 */
class FormantEnvelope
{
public:
    FormantEnvelope();

    /**
     * Generate spectral envelope for a given vowel shape
     * @param envelope Output buffer (size = fftSize/2 + 1)
     * @param vowel Vowel shape (formant frequencies/bandwidths)
     * @param intensity Controls peak sharpness (0.0 = wide, 1.0 = narrow)
     * @param sampleRate Sample rate in Hz
     * @param fftSize FFT size (2048 recommended)
     */
    void generateEnvelope(std::vector<float>& envelope,
                         const VowelShape& vowel,
                         float intensity,
                         float sampleRate,
                         int fftSize);

    /**
     * Get AA vowel shape (M2: hardcoded)
     * Source: Peterson & Barney (1952) - male speaker averages
     */
    static VowelShape getAAVowel();

    /**
     * Get all vowel shapes (for M3+)
     */
    static VowelShape getVowelAA();
    static VowelShape getVowelAH();
    static VowelShape getVowelEE();
    static VowelShape getVowelOH();
    static VowelShape getVowelOO();

private:
    /**
     * Convert intensity (0-1) to bandwidth scale factor
     * Low intensity = wide peaks, high intensity = narrow peaks
     */
    float intensityToBandwidthScale(float intensity);

    /**
     * Compute Gaussian peak contribution at frequency f
     * @param f Frequency in Hz
     * @param formant Formant center frequency and bandwidth
     * @param bandwidthScale Multiplier for bandwidth (from intensity)
     * @return Magnitude multiplier (0.0 - 2.0 typical range)
     */
    float gaussianPeak(float f, const Formant& formant, float bandwidthScale);
};
