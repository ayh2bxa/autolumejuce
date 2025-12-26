#include "AudioResampler.h"
#include <cmath>
#include <algorithm>

AudioResampler::AudioResampler()
    : AudioFX()
    , sourceRate(44100.0)
    , targetRate(16000.0)
    , resampleRatio(0.0)
    , timeAccumulator(0.0)
    , outputBufferSize(0)
    , lastOutputSampleCount(0)
{
    reset();
}

void AudioResampler::initialize(double sampleRate)
{
    AudioFX::initialize(sampleRate);
    sourceRate = sampleRate;
    targetRate = 16000.0;
    resampleRatio = targetRate / sourceRate;
    reset();
}

void AudioResampler::reset()
{
    AudioFX::reset();
    // Clear FIR filter delay line
    std::memset(delayLine, 0, FIR_NUM_TAPS * sizeof(float));
    delayIndex = 0;
    timeAccumulator = 0.0;
    prevFilteredSample = 0.0f;
    currFilteredSample = 0.0f;
}

int AudioResampler::resample(float *in, float *out, int numSamples)
{
    int outputSampleCount = 0;

    for (int i = 0; i < numSamples; ++i) {
        // Apply FIR anti-aliasing filter
        float filteredSample = applyFIR(in[i]);

        // Linear interpolation resampling
        currFilteredSample = filteredSample;

        // Accumulate time for resampling
        timeAccumulator += resampleRatio;

        // Generate output samples when time accumulator >= 1.0
        while (timeAccumulator >= 1.0) {
            // Linear interpolation between previous and current sample
            float frac = static_cast<float>(1.0 - (timeAccumulator - 1.0) / resampleRatio);
            frac = std::max(0.0f, std::min(1.0f, frac)); // Clamp to [0, 1]

            out[outputSampleCount++] = prevFilteredSample + frac * (currFilteredSample - prevFilteredSample);

            timeAccumulator -= 1.0;
        }

        prevFilteredSample = currFilteredSample;
    }

    lastOutputSampleCount = outputSampleCount;
    return outputSampleCount;
}

void AudioResampler::apply(float *in, float *out, int numSamples)
{
    resample(in, out, numSamples);
}

int AudioResampler::getExpectedOutputSize(int inputSamples) const
{
    return static_cast<int>(std::ceil(inputSamples * resampleRatio));
}

void AudioResampler::onSampleRateChanged()
{
    sourceRate = sampleRate;
    resampleRatio = targetRate / sourceRate;
    reset();
}

float AudioResampler::applyFIR(float inputSample)
{
    // Insert new sample into circular delay line
    delayLine[delayIndex] = inputSample;

    // Compute FIR filter output (convolution)
    float output = 0.0f;
    int idx = delayIndex;

    for (int i = 0; i < FIR_NUM_TAPS; ++i) {
        output += FIR_TAPS[i] * delayLine[idx];

        // Move backward through circular buffer
        idx = (idx == 0) ? (FIR_NUM_TAPS - 1) : (idx - 1);
    }

    // Advance delay line index
    delayIndex = (delayIndex + 1) % FIR_NUM_TAPS;

    return output;
}
