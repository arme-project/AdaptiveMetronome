/**
 * \file PluginProcessor.h
 * \brief Header file for the PluginProcessor class.
 *
 * This file contains the definition of the PluginProcessor class, which is responsible for processing audio and MIDI data in the adaptive metronome.
 * It handles the ensemble model, parameters, and the audio processing pipeline.
 */

#pragma once
#include <JuceHeader.h>
#include "EnsembleModel.h"
#include "TimingModelParametersGroup.h"

using AudioParameterFloatToUse = juce::AudioParameterFloat;

/**
 * @brief This class is the main processor for the Adaptive Metronome plugin.
 * 
 * It inherits from juce::AudioProcessor and manages the ensemble model, parameters, and audio.
 * 
 * It is very important to note that this class functions differently in FULL_SYSTEM mode 
 * compared to PLUGIN mode.
 * 
 * In PLUGIN mode, playback is started automatically by the host/DAW (reaper, etc). 
 * The ensemble model will typically handle production of the intro tones (on MIDI channel 16), before start the regular ensemble modelling and playback. 
 * 
 * In FULL_SYSTEM mode, the user must manually start playback by clicking the "Play" button in the GUI, or alternatively, playback can be started via an OSC message. 
 * In this case, playback is "primed" by setting the `manualPlaybackStarted` flag to true, and also the "waitingForFirstNote` flag in the ensemble model is set to true.
 * This means that the ensemble model will not start processing until the first note is received.
 * This allows the metronome to be played externally (currently from MaxMSP), and for modelling to only occur from the first actual note onwards. 
*/
class AdaptiveMetronomeAudioProcessor : public juce::AudioProcessor
{
public:
	//==============================================================================
	/**
	 * \brief Constructor for AdaptiveMetronomeAudioProcessor.
	 *
	 * Initialises the AudioProcessor and the apvts, which contains pointers to the
	 * parameters and their associated ParameterLayout. The ensemble model is also
	 * initialised here with the correct number of players.
	 *
	 */
	AdaptiveMetronomeAudioProcessor();

	/**
	 * \brief Destructor for AdaptiveMetronomeAudioProcessor.
	 *
	 * Cleans up the resources used by the processor and the ensemble model.
	 */
	~AdaptiveMetronomeAudioProcessor() override;

	EnsembleModel ensemble;
	ARMETimingModel::PhaseCorrectionModelParameters stdModelParams;

	// Parameters stored in APVTS
	juce::AudioProcessorValueTreeState apvts;

	//==============================================================================
	// Currently maximum number of players is limited to 4
	static const int MAX_PLAYERS = 4;

	//==============================================================================
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
	//==============================================================================
	/**
	 * \brief Sets the manual playing state of the adaptive metronome.
	 *
	 * \param shouldPlay A boolean indicating whether manual playing should be enabled.
	 */
	void setManualPlaying(bool shouldPlay); // used in standalone mode

	/**
	 * Prepares the audio processor to start playing audio.
	 *
	 * This function is called before starting playback or when the playback
	 * sample rate or block size changes. It initializes the ensemble model
	 * with the new sample rate, ensures the MIDI output buffer is appropriately
	 * sized, and resets playback state variables.
	 *
	 * @param sampleRate The new sample rate to use for playback.
	 * @param samplesPerBlock The number of samples in each audio block.
	 */
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	void releaseResources() override;

	/**
	 * \brief Process a block of audio and MIDI data.
	 *
	 * This method is called by JUCE to process a block of audio and MIDI data.
	 * It is responsible for passing the MIDI data to the EnsembleModel for processing
	 * and replacing the output MIDI buffer.
	 *
	 * \param buffer The audio buffer to be processed.
	 * \param midiMessages The MIDI buffer to be processed.
	 */
	void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

	//==============================================================================
	//==============================================================================
	bool hasEditor() const override;
	juce::AudioProcessorEditor* createEditor() override;

	//==============================================================================
	const juce::String getName() const override;
	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override;

	//==============================================================================
	void getStateInformation(juce::MemoryBlock& destData) override;
	void setStateInformation(const void* data, int sizeInBytes) override;

	//==============================================================================
	int getNumPrograms() override;
	int getCurrentProgram() override;
	void setCurrentProgram(int index) override;
	const juce::String getProgramName(int index) override;
	void changeProgramName(int index, const juce::String& newName) override;

	//==============================================================================
	bool hasDefaultConfigBeenChecked = false;

	EnsembleModel& loadMidiFile(const juce::File& file, int userPlayers);
	EnsembleModel& loadXmlFile(const juce::File& file);
	void resetEnsemble();

	// These are no longer used and will be removed
	//    std::vector < std::vector < AudioParameterFloatToUse* > > alphaParameters, betaParameters;
	//    std::vector < AudioParameterFloatToUse* > volumeParameters, delayParameters, mNoiseStdParameters, tkNoiseStdParameters;
	//    std::vector < juce::AudioParameterInt* > channelParameters;

	//==============================================================================
	// Programatically creates all parameters - called by AdaptiveMetronomeAudioProcessor-->apvts constructor
	juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
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

	//==============================================================================
	// This is a helper class that simplifies access to parameter values.
	// Using APVTS requires searching parameters by id.
	// This allows access to parameters using bracket indexing.
	// paramType can be AudioProcessorFloat or AudioProcessorInt
	template <typename paramType>
	class ParameterIndexGetter
	{
	private:
		// Reference to the ValueTreeState to search for parameter.
		juce::AudioProcessorValueTreeState& avpts;
		// String that preceedes first player index in parameter name
		juce::String before_I;
		// String after first player index (and before second player index) in parameter name
		juce::String after_I;

	public:
		ParameterIndexGetter(juce::AudioProcessorValueTreeState& apvts_ref,
			juce::StringRef before_i,
			juce::StringRef after_i) : avpts(apvts_ref),
			before_I(juce::String(before_i)),
			after_I(juce::String(after_i))
		{
		}

		~ParameterIndexGetter() {}

		// Allows alphaParameter(i, j) type access to alpha and beta parameters
		paramType* operator()(int i, int j)
		{
			juce::String matchString(before_I + juce::String(i) + after_I + juce::String(j));
			auto audioParamPtr = dynamic_cast<paramType*>(avpts.getParameter(juce::StringRef(matchString)));
			return audioParamPtr;
		}

		// Allows volumeParameter(i) type access to all other parameters
		paramType* operator()(int i)
		{
			juce::String matchString(before_I + juce::String(i) + after_I);
			auto audioParamPtr = dynamic_cast<paramType*>(avpts.getParameter(juce::StringRef(matchString)));
			return audioParamPtr;
		};
	};

	// Initialise getters for all parameters - Defined in processor constructor
	ParameterIndexGetter<AudioParameterFloatToUse> volumeParameter, delayParameter, mNoiseStdParameter, tkNoiseStdParameter;
	ParameterIndexGetter<juce::AudioParameterInt> channelParameter;
	ParameterIndexGetter<AudioParameterFloatToUse> alphaParameter, betaParameter;

	//==============================================================================
	/**
	 * \brief Flag that indicates whether playback has been manually started.
	 *
	 * This flag indicates that the system has been primed for playback, but the first note has not yet beenn received, and hence EnsembleModel is not yet doing any processing. 
	 * 
	 * This allows things like a metronome to be played externally, while the EnsembleModel is paused as long as @ref EnsembleModel::waitingForFirstNote is also set to true. 
	 * 
	 * When this is false, playback starts when the host/DAW starts playback, and the EnsembleModel will start processing immediately.
	 * 
	 * \return A boolean indicating whether the playback has been started manually.
	 */
	bool manualPlaybackStarted = false;

	/**
	 * \brief Flag that indicates whether playback should be started manually.
	 *
	 * This flag is used to indicate whether the playback should be started manually by the user, via OSC messaging or GUI buttong. 
	 * 
	 * When set to false, the plugin works in it's original form, and waits for playback to be started in the host/DAW. 
	 * 
	 * \return A boolean indicating whether the playback should be started manually.
	 */
#if FULL_SYSTEM
	bool useManualPlaybackStart = true;
#else
	bool useManualPlaybackStart = false;
#endif
private:
	//==============================================================================
	juce::MidiBuffer midiOutputBuffer;

	//==============================================================================
	bool wasPlaying = false;

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdaptiveMetronomeAudioProcessor)
};