#pragma once

#include "audiofx.h"
#include <cstring>
#include <cmath>

/**
 * AudioResampler - Resamples audio from 44.1 kHz to 16 kHz
 *
 * Uses a 64-tap FIR anti-aliasing filter (cutoff at 7.2 kHz) followed by
 * linear interpolation to downsample from 44100 Hz to 16000 Hz.
 *
 * Filter characteristics:
 * - 64 taps, Kaiser window (beta=8)
 * - Cutoff: 7200 Hz (0.9 * target Nyquist)
 * - Stopband attenuation: ~80 dB
 * - Group delay: 31.5 samples (0.714 ms @ 44.1 kHz)
 */
class AudioResampler : public AudioFX
{
public:
    AudioResampler();
    ~AudioResampler() override = default;

    void initialize(double sampleRate) override;
    void reset() override;

    /**
     * Apply anti-aliasing filter and resample from 44.1 kHz to 16 kHz
     *
     * @param in Input buffer at source sample rate (e.g., 44.1 kHz)
     * @param out Output buffer at target sample rate (16 kHz)
     * @param numSamples Number of input samples
     * @return Number of output samples written
     */
    int resample(float *in, float *out, int numSamples);

    // Override for compatibility with base class signature
    void apply(float *in, float *out, int numSamples) override;

    /**
     * Calculate the expected number of output samples for a given input size
     */
    int getExpectedOutputSize(int inputSamples) const;

    /**
     * Get the number of output samples from the last resample operation
     */
    int getLastOutputSampleCount() const { return lastOutputSampleCount; }

    double getSourceRate() const { return sourceRate; }
    double getTargetRate() const { return targetRate; }
    double getResampleRatio() const { return resampleRatio; }

protected:
    void onSampleRateChanged() override;

private:
    // ========================================================================
    // FIR ANTI-ALIASING FILTER COEFFICIENTS
    // ========================================================================
    // 64-tap FIR filter designed for 44.1 kHz -> 16 kHz resampling
    // Cutoff: 7200 Hz, Kaiser window (beta=8), ~80 dB stopband attenuation
    static constexpr int FIR_NUM_TAPS = 64;
    static constexpr float FIR_TAPS[FIR_NUM_TAPS] = {
         0.0000184784f, -0.0000071143f, -0.0000963332f, -0.0001462596f,
         0.0000180091f,  0.0003734039f,  0.0005192430f, -0.0000000000f,
        -0.0009897702f, -0.0013696611f, -0.0001297310f,  0.0021426840f,
         0.0030400929f,  0.0005345436f, -0.0040657142f, -0.0060149894f,
        -0.0014979200f,  0.0070431688f,  0.0110111194f,  0.0035014124f,
        -0.0115048512f, -0.0192846525f, -0.0074570493f,  0.0183993315f,
         0.0337949562f,  0.0156463642f, -0.0308599694f, -0.0652113602f,
        -0.0376749062f,  0.0678416378f,  0.2103109281f,  0.3121149084f,
         0.3121149084f,  0.2103109281f,  0.0678416378f, -0.0376749062f,
        -0.0652113602f, -0.0308599694f,  0.0156463642f,  0.0337949562f,
         0.0183993315f, -0.0074570493f, -0.0192846525f, -0.0115048512f,
         0.0035014124f,  0.0110111194f,  0.0070431688f, -0.0014979200f,
        -0.0060149894f, -0.0040657142f,  0.0005345436f,  0.0030400929f,
         0.0021426840f, -0.0001297310f, -0.0013696611f, -0.0009897702f,
        -0.0000000000f,  0.0005192430f,  0.0003734039f,  0.0000180091f,
        -0.0001462596f, -0.0000963332f, -0.0000071143f,  0.0000184784f,
    };

    /**
     * Apply FIR filter to a single sample
     */
    float applyFIR(float inputSample);

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    // Resampling parameters
    double sourceRate;          // Source sample rate (e.g., 44100 Hz)
    double targetRate;          // Target sample rate (e.g., 16000 Hz)
    double resampleRatio;       // targetRate / sourceRate
    double timeAccumulator;     // Accumulated time for resampling

    // FIR filter state
    float delayLine[FIR_NUM_TAPS];  // Circular delay line for FIR filter
    int delayIndex;                  // Current position in delay line

    // Linear interpolation state
    float prevFilteredSample;    // Previous filtered sample for interpolation
    float currFilteredSample;    // Current filtered sample for interpolation

    // Buffer management
    int outputBufferSize;        // Expected output buffer size
    int lastOutputSampleCount;   // Number of samples written in last resample
};
