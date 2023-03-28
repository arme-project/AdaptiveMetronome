#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AdaptiveMetronomeAudioProcessor::AdaptiveMetronomeAudioProcessor()
  : AudioProcessor (BusesProperties())
{
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
    manualPlaying = false;
    reaperPlaying = false;
}

inline const char* const BoolToString(bool b)
{
    return b ? "true" : "false";
}

void AdaptiveMetronomeAudioProcessor::setManualPlaying(bool shouldPlay)
{
    manualPlaying = shouldPlay;
    ensemble.waitingForFirstNote = true;
    DBG("SETTING PLAYBACK TO " << BoolToString(manualPlaying) << ", " << BoolToString(ensemble.waitingForFirstNote));
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
        bpm = 120.0; //playHeadPosition->getBpm();
    }
    
    double tempo = bpm.hasValue() ? *bpm : 60;
    
    //==========================================================================
    // Prepare output MIDI buffer
    midiOutputBuffer.clear();
    
    //==========================================================================
    // If playback has just stopped, stop all sound
    bool playbackStopped = !manualPlaying && wasPlaying;
    
    if (playbackStopped)
    {
        EnsembleModel::soundOffAllChannels (midiOutputBuffer);
    }
    
    //wasPlaying = playing;
    wasPlaying = manualPlaying;

    
    //==========================================================================
    // If the playhead is moving start processing MIDI
    if (manualPlaying)
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
