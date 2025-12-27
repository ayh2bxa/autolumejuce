#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "autolume.h"
#include "AudioResampler.h"
#include "defines.h"

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Public access for editor
    Autolume renderer;

private:
    // Audio resampler for downsampling (44.1 kHz -> 16 kHz)
    AudioResampler downsampler;

    // Reconstruction filter for upsampled audio (removes imaging artifacts)
    AudioResampler reconstructionFilter;

    // Buffer for mono mixed audio (before resampling)
    std::array<float, Constants::max_buf_size> monoBuffer;

    // Buffer for resampled audio (16 kHz)
    std::vector<float> resampledBuffer;

    // Buffer for upsampled audio (before reconstruction filter)
    std::vector<float> upsampledBuffer;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
