#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <complex>

/**
 * STFTProcessor - Short-Time Fourier Transform processor with overlap-add
 *
 * Handles:
 * - FFT/IFFT using JUCE's juce::dsp::FFT
 * - Hann windowing
 * - Overlap-add synthesis
 * - Ring buffer management for block-based processing
 *
 * Configuration:
 * - FFT size: 2048 samples (good frequency resolution ~23 Hz at 48kHz)
 * - Hop size: 512 samples (75% overlap)
 * - Window: Hann (smooth, minimal spectral leakage)
 * - Latency: 1536 samples (FFT_SIZE - HOP_SIZE)
 */
class STFTProcessor
{
public:
    static constexpr int FFT_ORDER = 11;  // 2^11 = 2048
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;
    static constexpr int HOP_SIZE = 512;
    static constexpr int LATENCY_SAMPLES = FFT_SIZE - HOP_SIZE;  // 1536

    STFTProcessor();
    ~STFTProcessor();

    /**
     * Prepare for processing - allocates all buffers (RT-safe after this)
     * MUST be called before processBlock
     */
    void prepare(double sampleRate, int maxBlockSize);

    /**
     * Reset all buffers and state
     */
    void reset();

    /**
     * Process audio block with spectral processing callback
     * @param input Input audio buffer
     * @param output Output audio buffer
     * @param numSamples Number of samples to process
     * @param spectralCallback Function called for each FFT frame
     *        Parameters: (magnitudes, phases, numBins)
     *        Callback should modify magnitudes array in-place
     */
    using SpectralCallback = std::function<void(float* magnitudes, float* phases, int numBins)>;

    void processBlock(const float* input, float* output, int numSamples,
                     SpectralCallback spectralCallback);

    /**
     * Get latency in samples
     */
    int getLatencySamples() const { return LATENCY_SAMPLES; }

    /**
     * Get number of FFT bins (FFT_SIZE/2 + 1)
     */
    int getNumBins() const { return FFT_SIZE / 2 + 1; }

private:
    /**
     * Process one FFT frame
     */
    void processFrame(SpectralCallback& callback);

    /**
     * Apply Hann window to input buffer
     */
    void applyWindow(float* buffer, int size);

    /**
     * Convert complex FFT output to magnitude/phase
     */
    void complexToMagPhase(const std::complex<float>* complexData,
                          float* magnitudes, float* phases, int numBins);

    /**
     * Convert magnitude/phase to complex FFT input
     */
    void magPhaseToComplex(const float* magnitudes, const float* phases,
                          std::complex<float>* complexData, int numBins);

    // JUCE FFT object (order 11 = 2048 points)
    std::unique_ptr<juce::dsp::FFT> fft;

    // Sample rate
    double sampleRate = 48000.0;

    // Ring buffer for input samples
    juce::AudioBuffer<float> inputRingBuffer;
    int writePos = 0;

    // FFT buffers (preallocated, no RT allocations)
    std::vector<float> fftInput;          // Size: FFT_SIZE
    std::vector<float> fftOutput;         // Size: FFT_SIZE * 2 (interleaved real/imag)
    std::vector<std::complex<float>> fftComplex;  // Size: FFT_SIZE/2 + 1

    // Magnitude and phase buffers
    std::vector<float> magnitudes;        // Size: FFT_SIZE/2 + 1
    std::vector<float> phases;            // Size: FFT_SIZE/2 + 1

    // IFFT buffers
    std::vector<float> ifftInput;         // Size: FFT_SIZE * 2 (interleaved)
    std::vector<float> ifftOutput;        // Size: FFT_SIZE

    // Overlap-add buffer
    juce::AudioBuffer<float> overlapBuffer;  // Size: FFT_SIZE

    // Hann window (precomputed)
    std::vector<float> window;            // Size: FFT_SIZE

    // Frame counter (for hop size timing)
    int samplesUntilNextFrame = 0;
};
