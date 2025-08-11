#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "EnsembleModel.h"
#include <JucePluginDefines.h>

//==============================================================================
AdaptiveMetronomeAudioProcessor::AdaptiveMetronomeAudioProcessor()
	: AudioProcessor(BusesProperties()),
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
	
}

AdaptiveMetronomeAudioProcessor::~AdaptiveMetronomeAudioProcessor()
{
}

//==============================================================================
bool AdaptiveMetronomeAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
	juce::ignoreUnused(layouts);
	return true;
}

//==============================================================================
void AdaptiveMetronomeAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	ensemble.prepareToPlay(sampleRate);
	midiOutputBuffer.ensureSize(4096);

	wasPlaying = false;

	setManualPlaying(useManualPlaybackStart);
}

inline const char* const BoolToString(bool b)
{
	return b ? "true" : "false";
}

void AdaptiveMetronomeAudioProcessor::setManualPlaying(bool shouldPlay)
{
	manualPlaybackStarted = shouldPlay;
	ensemble.waitingForFirstNote = shouldPlay;
	DBG("SETTING PLAYBACK TO " << BoolToString(manualPlaybackStarted) << ", " << BoolToString(ensemble.waitingForFirstNote));
}

void AdaptiveMetronomeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	//==========================================================================
	// Check if playhead is currently moving and find the current BPM.
	bool playing = false;
	juce::Optional <double> bpm;
	auto playHeadPosition = getPlayHead()->getPosition();

	if (playHeadPosition.hasValue())
	{
		playing = playHeadPosition->getIsPlaying();
		//bpm = playHeadPosition->getBpm();
		bpm = 120.0;
	}

	double tempo = bpm.hasValue() ? *bpm : 60;

	//==========================================================================
	// Prepare output MIDI buffer
	midiOutputBuffer.clear();

	//==========================================================================
	// If playback has just stopped, stop all sound
	bool playbackStopped = !manualPlaybackStarted && wasPlaying;

	if (playbackStopped)
	{
		EnsembleModel::soundOffAllChannels(midiOutputBuffer);
	}

	//wasPlaying = playing;
	wasPlaying = manualPlaybackStarted;

	//==========================================================================
	// If the playhead is moving start processing MIDI
	if (manualPlaybackStarted)
	{
		ensemble.processMidiBlock(midiMessages, midiOutputBuffer, buffer.getNumSamples(), tempo);
	}

	//==========================================================================
	// Replace output MIDI buffer
	midiMessages.swapWith(midiOutputBuffer);
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
	return new AdaptiveMetronomeAudioProcessorEditor(*this);
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
void AdaptiveMetronomeAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
}

void AdaptiveMetronomeAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
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

void AdaptiveMetronomeAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String AdaptiveMetronomeAudioProcessor::getProgramName(int index)
{
	return {};
}

void AdaptiveMetronomeAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
EnsembleModel& AdaptiveMetronomeAudioProcessor::loadMidiFile(const juce::File& file, int userPlayers)
{
	ensemble.loadMidiFile(file, userPlayers);

	return ensemble;
}

//==============================================================================
EnsembleModel& AdaptiveMetronomeAudioProcessor::loadXmlFile(const juce::File& file)
{
	ensemble.GetConfigHandler()->LoadConfig(file);

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

juce::AudioProcessorValueTreeState::ParameterLayout AdaptiveMetronomeAudioProcessor::createParameterLayout()
{
	float defaultAlpha = 0.1f;
	float defaultBeta = 0.1f;
	float defaultVolume = 1.0f;
	float defaultTkNoise = 1.0f;
	float defaultMNoise = 0.1f;
	float defaultDelay = 0.0f;

	juce::AudioProcessorValueTreeState::ParameterLayout params;

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		// Volume
		params.add(std::make_unique<AudioParameterFloatToUse>("player" + juce::String(i) + "-volume",
			"Player " + juce::String(i) + " Volume",
			0.0, 1.0, defaultVolume));

		// Channel
		params.add(std::make_unique<juce::AudioParameterInt>("player" + juce::String(i) + "-channel",
			"Player " + juce::String(i) + " MIDI Channel",
			1, 16, (i + 1)));

		// Delay
		params.add(std::make_unique<AudioParameterFloatToUse>("player" + juce::String(i) + "-delay",
			"Player " + juce::String(i) + " Delay",
			0.0, 200.0, defaultDelay));

		// Motor Noise
		params.add(std::make_unique<AudioParameterFloatToUse>("player" + juce::String(i) + "-mnoise-std",
			"Player " + juce::String(i) + " Motor Noise Std",
			0.0, 10.0, defaultMNoise));

		// Timekeeper Noise
		params.add(std::make_unique<AudioParameterFloatToUse>("player" + juce::String(i) + "-tknoise-std",
			"Player " + juce::String(i) + " Time Keeper Noise Std",
			0.0, 50.0, defaultTkNoise));

		// Inner player loop for alphas
		for (int j = 0; j < MAX_PLAYERS; j++)
		{
			// Alpha
			params.add(std::make_unique<AudioParameterFloatToUse>("alpha-" + juce::String(i) + "-" + juce::String(j),
				"Alpha " + juce::String(i) + "-" + juce::String(j),
				0.0, 1.0, defaultAlpha));

			// Beta
			params.add(std::make_unique<AudioParameterFloatToUse>("beta-" + juce::String(i) + "-" + juce::String(j),
				"Beta " + juce::String(i) + "-" + juce::String(j),
				0.0, 1.0, defaultBeta));
		}
	}
	return params;
}