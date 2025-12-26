#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    // Initialize the Autolume renderer (model loading, GPU setup, thread start)
    renderer.initialize();

    // Initialize the audio resampler
    resampler.initialize(sampleRate);

    // Allocate buffers
    monoBuffer.resize(samplesPerBlock);

    // Calculate expected output size for resampled buffer
    int expectedResampledSize = resampler.getExpectedOutputSize(samplesPerBlock);
    resampledBuffer.resize(expectedResampledSize + 64); // Extra padding for safety
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that didn't have corresponding input channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    int numSamples = buffer.getNumSamples();

    // Ensure buffers are large enough
    if (monoBuffer.size() < static_cast<size_t>(numSamples))
        monoBuffer.resize(numSamples);

    // Mix to mono (average left and right channels)
    auto* leftData = buffer.getReadPointer(0);
    auto* rightData = totalNumInputChannels > 1 ? buffer.getReadPointer(1) : leftData;

    for (int s = 0; s < numSamples; ++s) {
        monoBuffer[s] = 0.5f * (leftData[s] + rightData[s]);
    }

    // Apply anti-aliasing filter and resample from 44.1 kHz to 16 kHz
    int numResampledSamples = resampler.resample(monoBuffer.data(), resampledBuffer.data(), numSamples);

    // TODO: Send resampledBuffer to the Autolume renderer for processing
    // The resampled audio is now in resampledBuffer[0..numResampledSamples-1]
    // at 16 kHz sample rate

    // Upsample back to 44.1 kHz using linear interpolation
    double upsampleRatio = static_cast<double>(numSamples) / static_cast<double>(numResampledSamples);

    auto* outputLeft = buffer.getWritePointer(0);
    auto* outputRight = totalNumOutputChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i) {
        // Calculate position in resampled buffer
        double resampledPos = static_cast<double>(i) / upsampleRatio;
        int index = static_cast<int>(resampledPos);
        float frac = static_cast<float>(resampledPos - index);

        // Linear interpolation
        float sample;
        if (index + 1 < numResampledSamples) {
            sample = resampledBuffer[index] + frac * (resampledBuffer[index + 1] - resampledBuffer[index]);
        } else {
            sample = resampledBuffer[index];
        }

        // Write to output (mono to stereo)
        outputLeft[i] = sample;
        if (outputRight != nullptr) {
            outputRight[i] = sample;
        }
    }
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
