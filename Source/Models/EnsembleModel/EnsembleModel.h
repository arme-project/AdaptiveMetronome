/**
 * \file EnsembleModel.h
 * \brief Header file for the EnsembleModel class.
 *
 * This file contains the definition of the EnsembleModel class, which is responsible for managing the ensemble of players, their states, and communication with the Max/MSP patch.
 * It handles loading MIDI files, processing MIDI data, and sending OSC messages to Max/MSP.
 */

#pragma once
/* STANDARD */
#include <JuceHeader.h>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>

/* CORE */
//#include "PluginProcessor.h"
// Forward Declare to avoid circular include. Need to check if this is still necessary
class AdaptiveMetronomeAudioProcessor;

/* MODEL */
#include "UserPlayer.h"
#include "Player.h"

/* HELPER */
#include "FlagLock.h"
#include "Poller.h"
#include "Logger.h"
#include "ConfigHandler.h"
#include "OSCHandler.h"


using std::function;


// Allows AudioParameter that's used to be replaced with a custom implementation. No longer used.
using AudioParameterFloatToUse = juce::AudioParameterFloat;

/**
 * \class EnsembleModel
 * \brief The EnsembleModel class manages the ensemble of players, their states, and communication with the Max/MSP patch.
 *
 * This class is responsible for loading MIDI files, processing MIDI data, and sending OSC messages to Max/MSP.
 * It also handles the playback of introductory tones and the management of player parameters.
 */
class EnsembleModel
{
public:

	//====================
	//| PUBLIC VARIABLES |
	//====================

	AdaptiveMetronomeAudioProcessor* processor = nullptr; // Pointer to the Adaptive Metronome Processor owner
	bool OSCAutoConnect = true; // Defaulted to autoconnect to port 8000 and 8001
	bool waitingForFirstNote = true;

	// TODO: Change to a JUCE parameter
	juce::Atomic<int> currentNoteIndex;

	//===============================================
	//| CREATION AND DESTRUCTION OF ENSEMBLE OBJECT |
	//===============================================

	/**
	 * \brief Constructor for the EnsembleModel class.
	 * Initialises the ensemble model with a pointer to the processor and sets up
	 * the logger, poller, config handler, and OSC handler.
	 *
	 * \param processorPtr Pointer to the AdaptiveMetronomeAudioProcessor instance.
	 */
	EnsembleModel(AdaptiveMetronomeAudioProcessor* processorPtr);

	/**
	 * \brief Destructor for the EnsembleModel class.
	 * Stops the poller and logger threads to ensure clean shutdown.
	 */
	~EnsembleModel();

	/**
	 * \brief Releases any resources used by the ensemble model.
	 *
	 * This function is called when the ensemble model is no longer needed. This function is
	 * currently not implemented.
	 */
	void releaseResources();

	//====================
	//| GETTER FUNCTIONS |
	//====================

	// Getters for Player Parameters
	juce::AudioParameterInt& getPlayerChannelParameter(int playerIndex);
	AudioParameterFloatToUse& getPlayerDelayParameter(int playerIndex);
	AudioParameterFloatToUse& getPlayerMotorNoiseParameter(int playerIndex);
	AudioParameterFloatToUse& getPlayerTimeKeeperNoiseParameter(int playerIndex);
	AudioParameterFloatToUse& getPlayerVolumeParameter(int playerIndex);
	AudioParameterFloatToUse& getAlphaParameter(int player1Index, int player2Index);
	AudioParameterFloatToUse& getBetaParameter(int player1Index, int player2Index);

	// Getters for Ensemble Parameters
	int getNumPlayers();
	int getNumUserPlayers();
	bool isPlayerUserOperated(int playerIndex);
	juce::String GetFileNameOverride();

	// Manual Playing
	bool IsManuallyPlaying();
	void SetManualPlaying(bool isPlaying);

	// Handler Functions
	ConfigHandler* GetConfigHandler() const;
	Logger* GetLogger() const;

	// MIDI file
	juce::MidiFile GetMidiFile();

	//====================
	//| SETTER FUNCTIONS |
	//====================

	void setAlphaBetaParams(float valueIn);
	void SetConfigFileNameOverride(juce::String filename);
	void SetLogSubFolder(juce::String newLogSubFolder);
	void SetNumUserPlayers(int numPlayers);
	void SetNumIntroTones(int numIntroTonesIn);

	void SetTimekeeperNoiseSTD(int index, double value);
	void SetMotorNoiseSTD(int index, double value);
	void SetAlphaParam(int i, int j, double value);
	void SetBetaParam(int i, int j, double value);

	//======================
	//| OPEN SOUND CONTROL |
	//======================

	/**
	* \brief This synchronises both Max MSP and the Adapative Metronone.
	*
	* This method is called from the PluginProcessor when a note is played via OSC from Max
	* oscOnsetTime is in seconds, msMax is the time of the onset in ms according to Max's cpu clock.
	*
	* \param oscOnsetTime The onset time from the OSC message, in seconds.
	* \param onsetNoteNumber The note number of the note that was played.
	* \param msMax The time of the onset in ms according to Max's cpu clock.
	*/
	void setUserOnsetFromOsc(float oscOnsetTime, int onsetNoteNumber, int msMax);
	
	/*
	* \brief Connects the OSC sender to the specified port number or 8001 by default if nothing is provided..
	*/
	void ConnectOSCReceiver(int portNumber);
	
	/*
	* \brief Sends an action message via OSC with the specified message string.
	*
	* \param message The message string to send.
	*/
	void SendActionMessage(juce::String message);

	//============================
	//| PREPERATION AND PLAYBACK |
	//============================

	/**
	*  \brief Loads a MIDI file into the ensemble model.
	*
	*  The file is read into a juce::MidiFile, and then the tracks are converted into
	*  Player objects. The userPlayers parameter determines how many of the tracks
	*  are set to be user-operated. The method returns false if the file cannot be
	*  read, or if the playersInUse flag is set.

	*   \param file The MIDI file to load.
	*   \param userPlayers The number of user players tracks in the file.
	*   \return Whether the file was loaded successfully.
	*/
	bool loadMidiFile(const juce::File& file, int userPlayers);

	/**
	* \brief Create a Player for each track in the file which has note on events.
	*
	* The first 'numUserPlayers' will be UserPlayers, and the rest will be
	* Players.
	*
	* \param file The MidiFile from which to create the players
	*/
	void createPlayers(const juce::MidiFile& file);

	/**
	* \brief Sets the sample rate of the ensemble model.
	*
	* \param newSampleRate The new sample rate to set.
	*/
	void prepareToPlay(double newSampleRate);

	// RESET FUNCTIONS

	/**
	 * \brief Resets all players to the beginning of their scores, and resets the onset times of all players to 0.0.
	 * \return Returns false if the players are currently in use.
	 */
	bool reset();

	/**
	 * Resets the model to its initial state, and setting the number of Intro Tones.
	 *
	 * \param skipIntroNotes UNUSED
	 *
	 * \return false always.
	 */
	bool reset(bool skipIntroNotes);


	/**
	* \brief Sets the tempo of the ensemble to the given beats per minute (bpm).
	*
	* If the tempo has not actually changed, the function returns without doing anything.
	*
	* Otherwise, it updates the samplesPerBeat variable of the ensemble, and calls the
	* setInitialPlayerTempo() function to set the initial onset interval of each player to
	* the new tempo.
	*
	* \param bpm the tempo in beats per minute
	*/
	void setTempo(double bpm);


	/**
	* \brief Sends messages to turn off all notes, sounds and controllers on all
	* MIDI channels to the given MidiBuffer. This is useful for stopping
	* all sound when the user closes the plugin or changes presets.
	*
	* \param midi The MidiBuffer to send messages to.
	*/
	static void soundOffAllChannels(juce::MidiBuffer& midi);

	/**
	* \brief Checks if a MIDI sequence contains any note on events.
	*
	* This method can be used to check if a \ref juce::MidiMessageSequence contains any note on events. If it does, the method returns true, otherwise it returns false.
	*
	* \param seq The MIDI sequence to check.
	*
	* \return true if the sequence contains any note on events, false otherwise.
	*/
	static bool checkMidiSequenceHasNotes(const juce::MidiMessageSequence* seq);

	/**
	* \brief Trigger the first note to be played by the player.
	*
	* \details Sets waitingForFirstNote to false.
	*/
	void triggerFirstNote();

	//=================
	//| MAIN FUNCTION |
	//=================

	/**
	* \brief Processes a block of incoming MIDI data and generates corresponding output MIDI data.
	*
	* This function updates the tempo based on the DAW playhead and processes each sample
	* of the MIDI buffer. If the ensemble has been reset, it clears the output MIDI buffer.
	* It handles the playback of introductory tones and user intros before proceeding to
	* play the main score for each sample in the buffer.
	*
	* \param inMidi The incoming MIDI data buffer.
	* \param outMidi The output MIDI data buffer.
	* \param numSamples The number of samples in the MIDI buffer.
	* \param tempo The current tempo in beats per minute.
	*/
	void processMidiBlock(const juce::MidiBuffer& inMidi,
		juce::MidiBuffer& outMidi,
		int numSamples,
		double tempo);

private:

	//=====================
	//| PRIVATE VARIABLES |
	//=====================

	// Pointers to corresponding helper classes
	std::unique_ptr<Logger> logger;
	std::unique_ptr<Poller> poller;
	std::unique_ptr<ConfigHandler> config;
	std::unique_ptr<OSCHandler> osc;

	// Defaulted to autoconnect OSC to ports 8000 and 8001
	bool oscAutoConnect = true;

	// File Names and Path
	juce::MidiFile midiFile;
	juce::String configSubfolder = "";

	// Timing Parameters
	double sampleRate = 44100.0;
	int samplesPerBeat = sampleRate / 4;
	int scoreCounter = 0;

	// Intro countdown
	const int introToneChannel = 16;
	int numIntroTones = 4;
	static const int introToneNoteFirst = 84;
	static const int introToneNoteOther = 72;
	static const juce::uint8 introToneVel = 100;
	int introCounter = 0;
	int introTonesPlayed = 0;
	bool playbackStarted = false;
	bool introFinishedPlaying = false;
	bool firstSampleProcessed = false;

	// Number of USER players in Ensemble
	int numUserPlayers = 1;
	std::vector<bool> isUserFlags;

	// Intro Tone
	bool initialTempoSet = false;

	//========================
	//| INTRO TONE FUNCTIONS |
	//========================

	/**
	 * \brief Plays the intro tones for the ensemble model.
	 *
	 * This function plays the intro tones for the ensemble model by checking the
	 * current intro counter and playing the appropriate note on or off event.
	 * The intro tones are played at a specific sample index, and the number of
	 * intro tones played is tracked.
	 *
	 * \param midi The MIDI buffer to which the intro tones are added.
	 * \param sampleIndex The sample index at which to play the intro tones.
	 */
	void playIntroTones(juce::MidiBuffer& midi, int sampleIndex);

	/**
	 * \brief Plays the intro tone for the ensemble model.
	 *
	 * The intro tone is played on the first channel, and the note number is given by
	 * introToneNoteFirst or introToneNoteOther depending on how many intro tones have been played.
	 *
	 * \param midi The MIDI buffer to which the intro tone should be added.
	 * \param sampleIndex The sample index at which to play the intro tone.
	 */
	void introToneOn(juce::MidiBuffer& midi, int sampleIndex);

	/**
	* \brief Turns off the intro tone for the ensemble model.
	 *
	 * The intro tone is turned off at the given sample index, and the note number is
	 * determined by whether it is the first or other intro tone.
	 *
	 * \param midi The MIDI buffer to which the note off message will be added.
	 * \param sampleIndex The sample index at which to turn off the intro tone.
	 */
	void introToneOff(juce::MidiBuffer& midi, int sampleIndex);

	// UNIMPLEMENTED FUNCTION
	// void introToneOnOff (juce::MidiBuffer &midi, juce::MidiMessage (*function)(int, int, juce::uint8), int sampleIndex);

	//==================
	//| MAIN FUNCTIONS |
	//==================
	//  Funtions for ammendinding timings for each player in this ensemble. These
	//  should only be called from within processMidiBlock().

	/**
	* \brief Sets the onset interval for each player in the ensemble to the current tempo.
	*
	* This is only done if the tempo has not yet been set. This is a one-time operation
	* that is used to set the initial tempo for the players. The tempo is set to the
	* current tempo of the processor, which is stored in the samplesPerBeat.
	*/
	void setInitialPlayerTempo();

	/**
	 * \brief Checks if all players have played a note and if
	 * there are new onset times available to be used for the next iteration of
	 * the ensemble model.
	 *
	 * \returns true if all players have played a note, false otherwise
	 */
	bool newOnsetsAvailable();

	/**
	* \brief Calculates new onset intervals for each player based on the most recent onset times of the other players and the parameters of the ensemble model.
	*
	* The new onset intervals are calculated by calling recalculateOnsetInterval on each
	* player, and then the next note time is calculated by adding the new interval to
	* the most recent onset time for each player. This information is then sent to the
	* Max/MSP patch through OSC messages. If logging is enabled, the details of the most
	* recent onsets are stored in buffers to be logged.
	*/
	void calculateNewIntervals();

	/**
	 * \brief Calculates new onset intervals for each player based on the most recent onset times of the other players and the parameters of the ensemble model.
	 *
	 * The new onset intervals are calculated by calling recalculateOnsetInterval on each
	 * player, and then the next note time is calculated by adding the new interval to
	 * the most recent onset time for the player. The next note time is then converted
	 * to milliseconds and sent to the Max/MSP patch to be displayed.
	 *
	 * The details of the most recent onsets are then stored in buffers to be logged.
	 */
	void clearOnsetsAvailable();

	/**
	* \brief Store the log information about the latest onset from the given player in the
	* logging buffers.
	*
	* \param bufferIndex The index of the logging buffer to store the data in.
	* \param playerIndex The index of the player whose onset details are to be stored.
	*/
	void storeOnsetDetailsForPlayer(int playerIndex, Logger::LogData& log);


	/**
	* @brief Get the Latest Alphas object (NOT IMPLEMENTED)
	*/
	void getLatestAlphas();

	///**
	//* \brief Sets the tempo of the ensemble to the given beats per minute (bpm).
	//*
	//* If the tempo has not actually changed, the function returns without doing anything.
	//*
	//* Otherwise, it updates the samplesPerBeat variable of the ensemble, and calls the
	//* setInitialPlayerTempo() function to set the initial onset interval of each player to
	//* the new tempo.
	//*
	//* \param bpm the tempo in beats per minute
	//*/
	//void setTempo (double bpm);

	//===================================
	//| ENSEMBLE PLAYERS AND PARAMETERS |
	//===================================
	// The following functions should only be called when the playersInUse
	// flag has been locked using the above FlagLock class.

	std::vector<std::unique_ptr<Player>> players;
	std::atomic_flag playersInUse;
	std::atomic_flag resetFlag;

	/**
	 * @brief Initialises a matrix of alpha and beta parameters base on the total number of players there are
	 */
	void createAlphaBetaParameters();

	/**
	 * \brief Processes a block of MIDI data and updates the state of the EnsembleModel.
	 *
	 * \param inMidi The input MIDI buffer.
	 * \param outMidi The output MIDI buffer.
	 * \param sampleIndex The sample index at which the block of MIDI data should be processed.
	 */
	void playScore(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int sampleIndex);
	
	/**
	 * \brief Play an intro tone for each user-operated player.
	 *
	 * The intro tone is played at the given
	 * sampleIndex, and the note number is given by introToneNoteOther.
	 *
	 * \param inMidi The input MIDI buffer.
	 * \param outMidi The output MIDI buffer.
	 * \param sampleIndex The sample index at which to play the intro tone.
	 */
	void playUserIntro(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int sampleIndex);
	
	/**
	 * \brief Resets all players in the ensemble model
	 *
	 * Initialising intro countdown, score counter and tempo. Also starts loops for logging onset times and polling for
	 * new alpha values.
	 */
	void resetPlayers();

	/**
	 * @brief NOT IMPLEMENTED
	 *
	 * @param onsets
	 * @param delays
	 */
	void postLatestOnsets(const std::vector<int>& onsets, const std::vector<int>& delays);
};
