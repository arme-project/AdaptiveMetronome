/**
 * \file EnsembleModel.cpp
 * \brief Source file for the EnsembleModel class.
 *
 * This file contains the implementation of the EnsembleModel class, which is responsible for managing the ensemble of players, their states, and communication with the Max/MSP patch.
 * It handles loading MIDI files, processing MIDI data, and sending OSC messages to Max/MSP.
 */

#include <JuceHeader.h>
#include "EnsembleModel.h"
#include "PluginProcessor.h"

using namespace std::chrono;
using namespace std::chrono_literals;

//==============================================================================
// PUBLIC FUNCTIONS
//==============================================================================

#pragma region CREATION AND DESTRUCTION OF ENSEMBLE OBJECT

// EnsembleModel Constructor
EnsembleModel::EnsembleModel(AdaptiveMetronomeAudioProcessor* processorPtr)
	: processor(processorPtr)
{
	// Creating Poller and Logger objects for tracking Player Paramaters throughout playback
	logger = std::make_unique<Logger>(players.size(), sampleRate, isUserFlags);
	poller = std::make_unique<Poller>(players.size());

	// Creating ConfigHanlder Object to be responsible for handling XML Loading or Saving
	config = std::make_unique<ConfigHandler>(this);
	osc = std::make_unique<OSCHandler>(this);

	playersInUse.clear();
	resetFlag.clear();
	currentNoteIndex.set(99);

	if (oscAutoConnect)
	{
		osc->ConnectSender();
		osc->ConnectReceiver();
	}
}

//=============================================================================
// Destructor for EnsembleModel as it stops the poller and logger threads to ensure clean shutdown.
EnsembleModel::~EnsembleModel()
{
	poller->Stop();
	logger->Stop();
}

//=============================================================================
// Releases resources used by the EnsembleModel.
void EnsembleModel::releaseResources() { return; }

/**
 * \brief Sets the alpha parameters for all player pairs in the processor.
 *
 * \param valueIn The value to set for the alpha parameters.
 */
void EnsembleModel::setAlphaBetaParams(float valueIn)
{
	for (int i = 0; i < processor->MAX_PLAYERS; i++)
	{
		for (int j = 0; j < processor->MAX_PLAYERS; j++)
		{
			*processor->alphaParameter(i, j) = valueIn;
		}
	}
}

#pragma endregion

//==============================================================================

#pragma region GETTER FUNCTIONS

 // Returns the filename override for the logger.
juce::String EnsembleModel::GetFileNameOverride()
{
	return logger->GetFileNameOverride();
}

// Returns whenever Manual Playing is enabled or not.
bool EnsembleModel::IsManuallyPlaying()
{
	return processor->manualPlaying;
}

// Returns the MIDI file associated with the ensemble model.
juce::MidiFile EnsembleModel::GetMidiFile()
{
	return midiFile;
}

/**
* \brief Gets the number of players in the ensemble.
*/
int EnsembleModel::getNumPlayers()
{
	return static_cast <int> (players.size());
}

/**
 * \brief Gets the number of user players in the ensemble.
 */
int EnsembleModel::getNumUserPlayers()
{
	return static_cast <int> (numUserPlayers);
}

/**
 * \brief Checks if a player is user operated or not.
 */
bool EnsembleModel::isPlayerUserOperated(int playerIndex)
{
	return players[playerIndex]->isUserOperated();
}

juce::AudioParameterInt& EnsembleModel::getPlayerChannelParameter(int playerIndex)
{
	return *processor->channelParameter(playerIndex);
}

AudioParameterFloatToUse& EnsembleModel::getPlayerDelayParameter(int playerIndex)
{
	return *processor->delayParameter(playerIndex);
}

AudioParameterFloatToUse& EnsembleModel::getPlayerMotorNoiseParameter(int playerIndex)
{
	return *processor->mNoiseStdParameter(playerIndex);
}

AudioParameterFloatToUse& EnsembleModel::getPlayerTimeKeeperNoiseParameter(int playerIndex)
{
	return *processor->tkNoiseStdParameter(playerIndex);
}

AudioParameterFloatToUse& EnsembleModel::getPlayerVolumeParameter(int playerIndex)
{
	return *processor->volumeParameter(playerIndex);
}

AudioParameterFloatToUse& EnsembleModel::getAlphaParameter(int player1Index, int player2Index)
{
	return *processor->alphaParameter(player1Index, player2Index);
}

AudioParameterFloatToUse& EnsembleModel::getBetaParameter(int player1Index, int player2Index)
{
	return *processor->betaParameter(player1Index, player2Index);
}

/**
* \brief Gets the ConfigHandler instance.
*/
ConfigHandler* EnsembleModel::GetConfigHandler() const
{
	return config.get();
}

/**
* \brief Gets the Logger instance.
*/
Logger* EnsembleModel::GetLogger() const
{
	return logger.get();
}

#pragma endregion

//==============================================================================

#pragma region SETTER FUNCTIONS

void EnsembleModel::SetConfigFileNameOverride(juce::String filename)
{
	logger->SetFilenameOverride(filename);
}

void EnsembleModel::SetLogSubFolder(juce::String newLogSubFolder)
{
	logger->SetSubFolder(newLogSubFolder);
}

void EnsembleModel::SetNumUserPlayers(int numPlayers)
{
	numUserPlayers = numPlayers;
}

void EnsembleModel::SetNumIntroTones(int numIntroTonesIn)
{
	numIntroTones = numIntroTonesIn;
}

void EnsembleModel::SetTimekeeperNoiseSTD(int index, double value)
{
	*processor->tkNoiseStdParameter(index) = value;
}

void EnsembleModel::SetMotorNoiseSTD(int index, double value)
{
	*processor->mNoiseStdParameter(index) = value;
}

void EnsembleModel::SetAlphaParam(int i, int j, double value)
{
	*processor->alphaParameter(i, j) = value;
}

void EnsembleModel::SetBetaParam(int i, int j, double value)
{
	*processor->betaParameter(i, j) = value;
}

void EnsembleModel::SetManualPlaying(bool isPlaying)
{
	processor->setManualPlaying(true);
}

#pragma endregion

//==============================================================================

#pragma region OPEN SOUND CONTROL

// Sets the user onset time from an OSC message, which is used to set the onset time for the user-operated players.
void EnsembleModel::setUserOnsetFromOsc(float oscOnsetTime, int onsetNoteNumber, int msMax)
{
	for (auto& player : players)
	{
		if (player->isUserOperated()) {
			//auto tickOfOnset = clock->convertMsMaxToTick(msMax);
			//int msOnsetSinceFirstSample = clock->getDurationSincePlayback(tickOfOnset);
			//int onsetInSamples = (int)(msOnsetSinceFirstSample * (sampleRate / 1000));

			// This is a simplified version of the above, as we are not using the clock anymore. Onset time is simply whenever the note is received
			int onsetInSamples = scoreCounter;
			int onsetInSamplesFromOnsetTime = oscOnsetTime * sampleRate;
			float errorInOnset = (onsetInSamples - onsetInSamplesFromOnsetTime) / (float)sampleRate;
			player->setOscOnsetTime(oscOnsetTime, onsetNoteNumber, onsetInSamples);
		}
	}
}

//==============================================================================
// Connects the OSC sender to the specified port number or 8001 by default if nothing is provided..
void EnsembleModel::ConnectOSCReceiver(int portNumber)
{
	osc->ConnectReceiver(portNumber);
}

//==============================================================================
// Sends an action message via OSC with a given message.
void EnsembleModel::SendActionMessage(juce::String message)
{
	osc->SendActionMessage(message);
}
#pragma endregion

//==============================================================================

#pragma region PREPERATION AND PLAYBACK

// Loads a MIDI file and creates players for each track in the file.
bool EnsembleModel::loadMidiFile(const juce::File& file, int userPlayers)
{
	FlagLock lock(playersInUse);

	if (!lock.locked)
	{
		return false;
	}

	//==========================================================================
	// Read in content of MIDI file.
	juce::FileInputStream inStream(file);

	if (!inStream.openedOk())
		return false; // put some error handling here

	int fileType = 0;

	if (!midiFile.readFrom(inStream, true, &fileType))
		return false; // more error handling

	midiFile.convertTimestampTicksToSeconds();

	//==========================================================================
	// Create player for each track in the file.
	numUserPlayers = userPlayers;
	createPlayers(midiFile); // create new players
	resetPlayers();

	return true;
}

//==============================================================================
// Creates players for each track in the MIDI file, ensuring that only tracks with note on events are included.
void EnsembleModel::createPlayers(const juce::MidiFile& file)
{
	//==========================================================================
	// Delete Old Players
	players.clear();

	//==========================================================================
	// Create a Player for each track in the file which has note on events.
	int nTracks = file.getNumTracks();
	int playerIndex = 0;

	for (int i = 0; i < nTracks; ++i)
	{
		auto track = file.getTrack(i);

		if (checkMidiSequenceHasNotes(track))
		{
			// Assing channels to players in a cyclical manner.
			int channelToUse = (playerIndex % 16) + 1;

			if (playerIndex < numUserPlayers)
			{
				players.push_back(std::make_unique <UserPlayer>(playerIndex++,
					track,
					channelToUse,
					sampleRate,
					scoreCounter,
					samplesPerBeat,
					processor));
			}
			else
			{
				players.push_back(std::make_unique <Player>(playerIndex++,
					track,
					channelToUse,
					sampleRate,
					scoreCounter,
					samplesPerBeat,
					processor));
			}
		}
	}

	// Tracks which is user players or not for logging
	for (const auto& player : players) {
		isUserFlags.push_back(player->isUserOperated());
	}

	// Initialises the logger object used for the logging with the newly created players
	if (!logger || getNumPlayers() != players.size()) {
		if (logger) logger->Stop();
		logger = std::make_unique<Logger>(players.size(), sampleRate, isUserFlags);
		logger->Start();
	}

	// Initialises the Poller object used for the polling with the newly created players
	if (!poller || getNumPlayers() != players.size()) {
		if (poller) poller->Stop();
		poller = std::make_unique<Poller>(players.size());
		poller->Start();
	}

	//==========================================================================
	createAlphaBetaParameters(); // create matrix of parameters for alphas
}

//==============================================================================
// Prepares the ensemble model for playback by setting the sample rate.
void EnsembleModel::prepareToPlay(double newSampleRate)
{
	sampleRate = newSampleRate;
}

//==============================================================================
// Resets the ensemble model to its initial state, resetting the players in the ensemble.
bool EnsembleModel::reset()
{
	FlagLock lock(playersInUse);

	if (!lock.locked)
	{
		return false;
	}

	resetPlayers();

	return true;
}

//==============================================================================
// Calls the reset function to reset the ensemble model, and sets the introTonesPlayed to the number of intro tones.
bool EnsembleModel::reset(bool skipIntroNotes)
{
	reset();
	introTonesPlayed = numIntroTones;

	return false;
}

//==============================================================================
// Sets the tempo of the ensemble model based on the given beats per minute (bpm).
void EnsembleModel::setTempo(double bpm)
{
	int newSamplesPerBeat = 60.0 * sampleRate / bpm;

	// Check tempo has actually changed.
	if (newSamplesPerBeat == samplesPerBeat)
	{
		return;
	}

	// Update tempo of playback.
	samplesPerBeat = newSamplesPerBeat;

	setInitialPlayerTempo();
}

//==============================================================================
// Turns off all MIDI channels by sending all notes off, all sound off, and all controllers off messages for each channel.
void EnsembleModel::soundOffAllChannels(juce::MidiBuffer& midi)
{
	for (int channel = 1; channel <= 16; ++channel)
	{
		midi.addEvent(juce::MidiMessage::allNotesOff(channel), 0);
		midi.addEvent(juce::MidiMessage::allSoundOff(channel), 0);
		midi.addEvent(juce::MidiMessage::allControllersOff(channel), 0);
	}
}

//==============================================================================
// Checks if a MIDI sequence has any note on events, returning true if it does and false otherwise.
bool EnsembleModel::checkMidiSequenceHasNotes(const juce::MidiMessageSequence* seq)
{
	for (auto event : *seq)
	{
		if (event->message.isNoteOn())
		{
			return true;
		}
	}

	return false;
}

//==============================================================================
// Triggers the first note to be played by the ensemble model by setting the waitingForFirstNote flag to false.
void EnsembleModel::triggerFirstNote() {
	waitingForFirstNote = false;
}

#pragma endregion

//==============================================================================

#pragma region MAIN FUNCTION

// Processes a block of MIDI data and updates the state of the EnsembleModel and timing of each player.
void EnsembleModel::processMidiBlock(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int numSamples, double tempo)
{
	FlagLock lock(playersInUse);

	if (!lock.locked)
	{
		return;
	}

	//==============================================================================
	// Update tempo from DAW playhead.
	setTempo(tempo);

	//==============================================================================
	// Clear output if ensemble has been reset
	if (!resetFlag.test_and_set())
	{
		soundOffAllChannels(outMidi);
	}

	//==============================================================================
	// Process each sample of the buffer for each player.
	for (int i = 0; i < numSamples; ++i)
	{
		if (introTonesPlayed < numIntroTones)
		{
			playIntroTones(outMidi, i);
			playUserIntro(inMidi, outMidi, i);
			continue;
		}

		playScore(inMidi, outMidi, i);
	}
}

#pragma endregion






//==============================================================================
// PRIVATE FUNCTIONS
//==============================================================================

#pragma region INTRO TONE FUNCTIONS

// / Plays the intro tones for the ensemble model, which are used to signal the start of playback.
void EnsembleModel::playIntroTones(juce::MidiBuffer& midi, int sampleIndex)
{
	if (introCounter == 0)
	{
		introToneOn(midi, sampleIndex);
	}
	else if (introCounter == samplesPerBeat / 4)
	{
		introToneOff(midi, sampleIndex);
	}
	else if (introCounter >= samplesPerBeat - 1)
	{
		++introTonesPlayed;
		introCounter = -1;
	}

	++introCounter;
}

//==============================================================================
// Turns on the intro tone for the ensemble model, which is used to signal the start of playback.
void EnsembleModel::introToneOn(juce::MidiBuffer& midi, int sampleIndex)
{
	if (introTonesPlayed % 4 == 0)
	{
		midi.addEvent(juce::MidiMessage::noteOn(introToneChannel, introToneNoteFirst, introToneVel), sampleIndex);
	}
	else
	{
		midi.addEvent(juce::MidiMessage::noteOn(introToneChannel, introToneNoteOther, introToneVel), sampleIndex);
	}
}

//==============================================================================
// Turns off the intro tone for the ensemble model, which is used to signal the end of playback.
void EnsembleModel::introToneOff(juce::MidiBuffer& midi, int sampleIndex)
{
	if (introTonesPlayed % 4 == 0)
	{
		midi.addEvent(juce::MidiMessage::noteOff(introToneChannel, introToneNoteFirst, introToneVel), sampleIndex);
	}
	else
	{
		midi.addEvent(juce::MidiMessage::noteOff(introToneChannel, introToneNoteOther, introToneVel), sampleIndex);
	}
}
#pragma endregion

//==============================================================================

#pragma region MAIN FUNCTIONS
// Funtions for ammendinding timings for each player in this ensemble. 
// These should only be called from within processMidiBlock().

// Sets the initial player tempo for each player in the ensemble model.
void EnsembleModel::setInitialPlayerTempo()
{
	if (!initialTempoSet)
	{
		for (auto& player : players)
		{
			player->setOnsetInterval(samplesPerBeat);
		}

		initialTempoSet = true;
	}
}

//==============================================================================
// Checks if new onsets are available by checking if each player has played a note.
bool EnsembleModel::newOnsetsAvailable()
{
	bool available = true;

	for (auto& player : players)
	{
		available = available && player->hasNotePlayed();
	}

	return available;
}

//==============================================================================
// Calculates new onset intervals for each player based on the most recent onset times of the other players and the parameters of the ensemble model.
void EnsembleModel::calculateNewIntervals()
{
	//==========================================================================
	// Get most recent alphas
	getLatestAlphas();

	//==========================================================================
	// Calculate new onset times for players.
	// Make sure all non-user players update before the user players.
	for (int i = 0; i < players.size(); ++i)
	{
		if (!players[i]->isUserOperated())
		{
			//            players [i]->recalculateOnsetInterval (samplesPerBeat, players, alphaParams [i], betaParams [i]);
			players[i]->recalculateOnsetInterval(samplesPerBeat, players);
		}
	}

	for (int i = 0; i < players.size(); ++i)
	{
		if (players[i]->isUserOperated())
		{
			players[i]->recalculateOnsetInterval(samplesPerBeat, players);
		}
	}

	for (int i = 0; i < players.size(); ++i)
	{
		int onsetInterval = players[i]->getOnsetInterval();
		int onsetTime = players[i]->getLatestOnsetTime();
		int nextNoteTime = onsetTime + onsetInterval;
		int nextNoteTimeInMS = nextNoteTime * 1000 / sampleRate;
		osc->MessageSendNewInterval(i, players[i]->getCurrentNoteIndex() + 1, nextNoteTimeInMS);
	}

	//==========================================================================
	// Add details of most recent onsets to buffers to be logged.
	if (logger)
	{
		for (int i = 0; i < players.size(); ++i)
		{
			Logger::LogData log;
			storeOnsetDetailsForPlayer(i, log);
			logger->AddEntry(log);
		}
	}
}

//==============================================================================
// Clears the onsets available for all players in the ensemble model.
void EnsembleModel::clearOnsetsAvailable()
{
	for (auto& player : players)
	{
		player->resetNotePlayed();
	}
}

//==============================================================================
// Stores the details of the latest onset for a player in the log data.
void EnsembleModel::storeOnsetDetailsForPlayer(int playerIndex, Logger::LogData& log)
{
	auto* player = players[playerIndex].get();

	log.onsetTime = player->getLatestOnsetTime();
	log.onsetInterval = player->getOnsetInterval();
	log.userInput = player->isUserOperated();
	log.delay = player->getLatestOnsetDelay();
	log.motorNoise = player->getMotorNoise();
	log.timeKeeperNoise = player->getTimeKeeperNoise();
	log.tkNoiseStd = player->getTimeKeeperNoiseStd();
	log.mNoiseStd = player->getMotorNoiseStd();
	log.volume = player->getLatestVolume();

	log.asyncs.resize(players.size());
	log.alphas.resize(players.size());
	log.betas.resize(players.size());

	for (int j = 0; j < players.size(); ++j)
	{
		log.asyncs[j] = player->getLatestOnsetTime() - players[j]->getLatestOnsetTime();
		log.alphas[j] = getAlphaParameter(playerIndex, j).get();
		log.betas[j] = getBetaParameter(playerIndex, j).get();
	}
}

//==============================================================================
// Gets the latest alpha values for each player in the ensemble model.
void EnsembleModel::getLatestAlphas()
{
}

#pragma endregion

//==============================================================================

#pragma region ENSEMBLE PLAYERS AND PARAMETERS

// Creates the alpha and beta parameters for each player in the ensemble model.
void EnsembleModel::createAlphaBetaParameters()
{
	for (int i = 0; i < players.size(); ++i)
	{
		double alpha = 0.25;
		double beta = 0.1;

		for (int j = 0; j < players.size(); ++j)
		{
			*processor->alphaParameter(i, j) = alpha;
			*processor->betaParameter(i, j) = beta;
		}
	}
}

//==============================================================================
// Processes a block of MIDI data for each player in the ensemble model, updating their state and playing notes as necessary.
void EnsembleModel::playScore(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int sampleIndex)
{
	for (auto& player : players)
	{
		player->processSample(inMidi, outMidi, sampleIndex);
	}

	// If all players have played a note, update timings.
	if (newOnsetsAvailable())
	{
		calculateNewIntervals();
		clearOnsetsAvailable();
	}

	++scoreCounter;
}

//==============================================================================
// Processes the user intro sample for each player in the ensemble model, playing the intro tone if the player is user-operated.
void EnsembleModel::playUserIntro(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int sampleIndex)
{
	for (auto& player : players)
	{
		if (player->isUserOperated())
		{
			player->processIntroSample(inMidi, outMidi, sampleIndex, introToneNoteOther);
		}
	}
}

//==============================================================================
// Resets all players in the ensemble model, including the intro countdown, score counter, and tempo.
void EnsembleModel::resetPlayers()
{
	//==========================================================================
	// Initialise intro countdown
	introCounter = 0; //-sampleRate / 2;
	introTonesPlayed = 0;

	// Initialise score counter
	scoreCounter = 0;
	firstSampleProcessed = false;

	// make sure to update player tempo when playback starts
	initialTempoSet = false;

	// Start loop for logging onset times or each player
	logger->Start();

	// Start loop which polls for new alpha values
	poller->Start();

	//==========================================================================
	// reset all players
	for (auto& player : players)
	{
		player->reset();
	}

	resetFlag.clear();
}

//==============================================================================
// Posts the latest onsets for each player in the ensemble model, including the delays for each onset (NOT IMPLEMENTED).
void EnsembleModel::postLatestOnsets(const std::vector<int>& onsets, const std::vector<int>& delays)
{
}
#pragma endregion

//==============================================================================


//void EnsembleModel::getLatestAlphas()
//{
//	//    if (pollingFifo)
//	//    {
//	//        // Consume everything in the buffer, only using the most recent set of alphas.
//	//        auto reader = pollingFifo->read (pollingFifo->getNumReady());
//	//
//	//        for (int player1 = 0; player1 < pollingBuffer.size(); ++player1)
//	//        {
//	//            int player2 = 0;
//	//
//	//            int block1Start = std::max (reader.blockSize1 + reader.blockSize2 - static_cast <int> (players.size()), 0);
//	//
//	//            for (int i = block1Start; i < reader.blockSize1; ++i)
//	//            {
//	//                *(*alphaParams) [player1][player2++] = pollingBuffer [player1][reader.startIndex1 + i];
//	//            }
//	//
//	//            int block2Start = std::max (block1Start - reader.blockSize1, 0);
//	//
//	//            for (int i = block2Start; i < reader.blockSize2; ++i)
//	//            {
//	//                *(*alphaParams) [player1][player2++] = pollingBuffer [player1][reader.startIndex2 + i];
//	//            }
//	//        }
//	//    }
//}
