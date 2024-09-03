#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "EnsembleModel.h"

//==============================================================================
AdaptiveMetronomeAudioProcessor::AdaptiveMetronomeAudioProcessor()
  : AudioProcessor (BusesProperties()),
    ensemble(this),
    apvts(*this, nullptr, "PARAMETERS", createParameterLayout()),
    volumeParameter(apvts, "player", "-volume"),
    delayParameter(apvts, "player", "-delay"),
    mNoiseStdParameter(apvts, "player", "-mnoise-std"),
    tkNoiseStdParameter(apvts, "player", "-tknoise-std"),
    channelParameter(apvts, "player", "-channel"),
    alphaParameter(apvts, "alpha-", "-"),
    betaParameter(apvts, "beta-", "-"),
    stdModelParams(4)
{
    // Test only ... sets default Alpha and Beta parameters on initialisation
    ensemble.setAlphaBetaParams(0.25);

    //TestFloatParameters();
    //std::mt19937 gen(0);
    //std::normal_distribution<float> nd(0.0f, 1.0f);
    //float testParamValue1 = nd(gen);
    //float testParamValue2 = nd(gen);
    //float testParamValue3 = nd(gen);


    float testParamValue1 = stdModelParams.getFloatParameterByName("testRandomParameter");
    float testParamValue2 = stdModelParams.getFloatParameterByName("testRandomParameter");
    float testParamValue3 = stdModelParams.getFloatParameterByName("testRandomParameter");
}

void AdaptiveMetronomeAudioProcessor::TestFloatParameters()
{
    // Test parameters
    float testParamValue = stdModelParams.getFloatParameterByName("testParameter");
    std::atomic<float> newFloat = 5.999f;
    stdModelParams.overrideFloatParameterValue("testParameter", &newFloat);
    float testParamValue2 = stdModelParams.getFloatParameterByName("testParameter");
    newFloat = 8.4561f;
    float testParamValue3 = stdModelParams.getFloatParameterByName("testParameter");
    stdModelParams.setFloatParameterByName("testParameter", 17.16454f);
    float testParamValue4 = stdModelParams.getFloatParameterByName("testParameter");
    juce::AudioParameterFloat* parameter = alphaParameter(0, 0);
    std::function<float()> getFloat = [parameter]() { return parameter->get(); };
    if (true)
    {
        stdModelParams.addFloatParameter("testParameter2", getFloat);
    }
    float testParamValue5 = stdModelParams.getFloatParameterByName("testParameter2");
    ensemble.setAlphaBetaParams(0.55);
    float testParamValue6 = stdModelParams.getFloatParameterByName("testParameter2");
}

AdaptiveMetronomeAudioProcessor::~AdaptiveMetronomeAudioProcessor()
{
}

//==============================================================================
bool AdaptiveMetronomeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    juce::ignoreUnused (layouts);
    return true;
}

//==============================================================================
void AdaptiveMetronomeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    ensemble.prepareToPlay (sampleRate);
    midiOutputBuffer.ensureSize (4096);
    
    wasPlaying = false;
}

void AdaptiveMetronomeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    //==========================================================================
    // Check if playhead is currently moving and find the current BPM.
    bool playing = false;
    juce::Optional <double> bpm;
    auto playHeadPosition = getPlayHead()->getPosition();
    
    if (playHeadPosition.hasValue())
    {
        playing = playHeadPosition->getIsPlaying();
        bpm = playHeadPosition->getBpm();
    }
    
    double tempo = bpm.hasValue() ? *bpm : 60;
    
    //==========================================================================
    // Prepare output MIDI buffer
    midiOutputBuffer.clear();
    
    //==========================================================================
    // If playback has just stopped, stop all sound
    bool playbackStopped = !playing && wasPlaying;
    
    if (playbackStopped)
    {
        EnsembleModel::soundOffAllChannels (midiOutputBuffer);
    }
    
    wasPlaying = playing;
    
    //==========================================================================
    // If the playhead is moving start processing MIDI
    if (playing)
    {
        ensemble.processMidiBlock (midiMessages, midiOutputBuffer, buffer.getNumSamples(), tempo);
    }
    
    //==========================================================================
    // Replace output MIDI buffer
    midiMessages.swapWith (midiOutputBuffer);
}

void AdaptiveMetronomeAudioProcessor::releaseResources()
{
}

//==============================================================================
bool AdaptiveMetronomeAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AdaptiveMetronomeAudioProcessor::createEditor()
{
    return new AdaptiveMetronomeAudioProcessorEditor (*this, ensemble);
}

//==============================================================================
const juce::String AdaptiveMetronomeAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AdaptiveMetronomeAudioProcessor::acceptsMidi() const
{
    return true;
}

bool AdaptiveMetronomeAudioProcessor::producesMidi() const
{
    return true;
}

bool AdaptiveMetronomeAudioProcessor::isMidiEffect() const
{
    return true;
}

double AdaptiveMetronomeAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================
void AdaptiveMetronomeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
}

void AdaptiveMetronomeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
}

//==============================================================================
int AdaptiveMetronomeAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AdaptiveMetronomeAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AdaptiveMetronomeAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AdaptiveMetronomeAudioProcessor::getProgramName (int index)
{
    return {};
}

void AdaptiveMetronomeAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
EnsembleModel& AdaptiveMetronomeAudioProcessor::loadMidiFile (const juce::File &file, int userPlayers)
{
    ensemble.loadMidiFile (file, userPlayers);
    
    return ensemble;
}

//==============================================================================
EnsembleModel& AdaptiveMetronomeAudioProcessor::loadXmlFile(const juce::File& file)
{
    ensemble.loadConfigFromXml(file);

    return ensemble;
}

void AdaptiveMetronomeAudioProcessor::resetEnsemble()
{
    ensemble.reset();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AdaptiveMetronomeAudioProcessor();
}
