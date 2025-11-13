#include "STFTProcessor.h"
#include <cmath>
#include <algorithm>

STFTProcessor::STFTProcessor()
{
    fft = std::make_unique<juce::dsp::FFT>(FFT_ORDER);
}

STFTProcessor::~STFTProcessor()
{
}

void STFTProcessor::prepare(double sampleRate_, int maxBlockSize)
{
    sampleRate = sampleRate_;

    // Allocate ring buffer (double FFT size for safety)
    inputRingBuffer.setSize(1, FFT_SIZE * 2);
    inputRingBuffer.clear();
    writePos = 0;

    // Allocate FFT buffers
    fftInput.resize(FFT_SIZE, 0.0f);
    fftOutput.resize(FFT_SIZE * 2, 0.0f);  // Interleaved real/imag
    fftComplex.resize(FFT_SIZE / 2 + 1);

    magnitudes.resize(FFT_SIZE / 2 + 1, 0.0f);
    phases.resize(FFT_SIZE / 2 + 1, 0.0f);

    ifftInput.resize(FFT_SIZE * 2, 0.0f);
    ifftOutput.resize(FFT_SIZE, 0.0f);

    // Allocate overlap-add buffer
    overlapBuffer.setSize(1, FFT_SIZE);
    overlapBuffer.clear();

    // Precompute Hann window
    window.resize(FFT_SIZE);
    for (int i = 0; i < FFT_SIZE; ++i)
    {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / FFT_SIZE));
    }

    samplesUntilNextFrame = HOP_SIZE;
}

void STFTProcessor::reset()
{
    inputRingBuffer.clear();
    overlapBuffer.clear();
    writePos = 0;
    samplesUntilNextFrame = HOP_SIZE;

    std::fill(fftInput.begin(), fftInput.end(), 0.0f);
    std::fill(fftOutput.begin(), fftOutput.end(), 0.0f);
    std::fill(magnitudes.begin(), magnitudes.end(), 0.0f);
    std::fill(phases.begin(), phases.end(), 0.0f);
    std::fill(ifftInput.begin(), ifftInput.end(), 0.0f);
    std::fill(ifftOutput.begin(), ifftOutput.end(), 0.0f);
}

void STFTProcessor::processBlock(const float* input, float* output, int numSamples,
                                 SpectralCallback spectralCallback)
{
    auto* ringData = inputRingBuffer.getWritePointer(0);
    const int ringSize = inputRingBuffer.getNumSamples();

    // Process each sample
    for (int i = 0; i < numSamples; ++i)
    {
        // Write input to ring buffer
        ringData[writePos] = input[i];
        writePos = (writePos + 1) % ringSize;

        // Check if it's time to process a frame
        samplesUntilNextFrame--;
        if (samplesUntilNextFrame <= 0)
        {
            processFrame(spectralCallback);
            samplesUntilNextFrame = HOP_SIZE;
        }

        // Read from overlap-add buffer (with latency compensation)
        int readPos = (writePos - LATENCY_SAMPLES + ringSize) % ringSize;
        output[i] = overlapBuffer.getSample(0, readPos);
    }
}

void STFTProcessor::processFrame(SpectralCallback& callback)
{
    const int numBins = FFT_SIZE / 2 + 1;
    auto* ringData = inputRingBuffer.getReadPointer(0);
    const int ringSize = inputRingBuffer.getNumSamples();

    // Copy FFT_SIZE samples from ring buffer to FFT input
    int readPos = (writePos - FFT_SIZE + ringSize) % ringSize;
    for (int i = 0; i < FFT_SIZE; ++i)
    {
        fftInput[i] = ringData[readPos];
        readPos = (readPos + 1) % ringSize;
    }

    // Apply Hann window
    for (int i = 0; i < FFT_SIZE; ++i)
    {
        fftInput[i] *= window[i];
    }

    // Perform FFT (JUCE uses interleaved format)
    std::copy(fftInput.begin(), fftInput.end(), fftOutput.begin());
    fft->performRealOnlyForwardTransform(fftOutput.data());

    // Convert to magnitude/phase
    for (int i = 0; i < numBins; ++i)
    {
        float real = fftOutput[i * 2];
        float imag = fftOutput[i * 2 + 1];

        // Sanitize for NaN/Inf
        if (std::isnan(real) || std::isinf(real)) real = 0.0f;
        if (std::isnan(imag) || std::isinf(imag)) imag = 0.0f;

        magnitudes[i] = std::sqrt(real * real + imag * imag);
        phases[i] = std::atan2(imag, real);
    }

    // Call spectral processing callback (modifies magnitudes in-place)
    if (callback)
    {
        callback(magnitudes.data(), phases.data(), numBins);
    }

    // Sanitize magnitudes after callback
    for (int i = 0; i < numBins; ++i)
    {
        if (std::isnan(magnitudes[i]) || std::isinf(magnitudes[i]))
            magnitudes[i] = 0.0f;

        // Clamp to reasonable range (prevent excessive boost)
        magnitudes[i] = std::min(magnitudes[i], 100.0f);
    }

    // Convert back to complex
    for (int i = 0; i < numBins; ++i)
    {
        ifftInput[i * 2] = magnitudes[i] * std::cos(phases[i]);
        ifftInput[i * 2 + 1] = magnitudes[i] * std::sin(phases[i]);
    }

    // Perform IFFT
    fft->performRealOnlyInverseTransform(ifftInput.data());

    // Apply window and overlap-add
    auto* overlapData = overlapBuffer.getWritePointer(0);
    for (int i = 0; i < FFT_SIZE; ++i)
    {
        float sample = ifftInput[i] * window[i];

        // Sanitize output
        if (std::isnan(sample) || std::isinf(sample))
            sample = 0.0f;

        // Overlap-add (use ring buffer index)
        int overlapPos = (writePos - FFT_SIZE + i + inputRingBuffer.getNumSamples()) % inputRingBuffer.getNumSamples();
        if (overlapPos < overlapBuffer.getNumSamples())
        {
            overlapData[overlapPos] += sample * 0.5f;  // Scale down for overlap
        }
    }

    // Clear old overlap buffer sections
    int clearStart = writePos;
    int clearEnd = (writePos + HOP_SIZE) % overlapBuffer.getNumSamples();
    if (clearStart < clearEnd)
    {
        for (int i = clearStart; i < clearEnd; ++i)
            overlapData[i] = 0.0f;
    }
    else
    {
        for (int i = clearStart; i < overlapBuffer.getNumSamples(); ++i)
            overlapData[i] = 0.0f;
        for (int i = 0; i < clearEnd; ++i)
            overlapData[i] = 0.0f;
    }
}
