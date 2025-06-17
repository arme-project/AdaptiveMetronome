#include <JuceHeader.h>
#include "EnsembleModel.h"

using namespace std::chrono;
using namespace std::chrono_literals;

/**
* \brief Constructor for EnsembleModel.
*
* \param processorPtr The audio processor which owns this model.
*/
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

/**
* \brief Destructor for the EnsembleModel class.
* Stops the logger loop and polling loop to ensure proper cleanup of resources.
*/
EnsembleModel::~EnsembleModel()
{
	poller->Stop();
	logger->Stop();
}

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

juce::String EnsembleModel::GetFileNameOverride()
{
	return logger->GetFileNameOverride();
}

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

bool EnsembleModel::IsManuallyPlaying()
{
	return processor->manualPlaying;
}

void EnsembleModel::SetManualPlaying(bool isPlaying)
{
	processor->setManualPlaying(true);
}

//==============================================================================
// OSC Messaging

juce::MidiFile EnsembleModel::GetMidiFile()
{
	return midiFile;
}

//==============================================================================
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

/**
* \brief Trigger the first note to be played by the player.
*
* \details Sets waitingForFirstNote to false.
*/
void EnsembleModel::triggerFirstNote() {
	waitingForFirstNote = false;
}

/**
 * \brief Resets all players to the beginning of their scores, and resets the onset times of all players to 0.0.
 * \return Returns false if the players are currently in use.
 */
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

/**
 * Resets the model to its initial state, and setting the number of Intro Tones.
 *
 * @param skipIntroNotes UNUSED
 *
 * @returns false always.
 */
bool EnsembleModel::reset(bool skipIntroNotes)
{
	reset();
	introTonesPlayed = numIntroTones;

	return false;
}

//==============================================================================

/**
* \brief Sets the sample rate of the ensemble model.
*
* \param newSampleRate The new sample rate to set.
*/
void EnsembleModel::prepareToPlay(double newSampleRate)
{
	sampleRate = newSampleRate;
}

/**
 * \brief Releases any resources used by the ensemble model.
 *
 * This function is called when the ensemble model is no longer needed. This function is
 * currently not implemented.
 */
void EnsembleModel::releaseResources() { return; }

//==============================================================================
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

//==============================================================================
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

//==============================================================================

/**
* \brief Sends messages to turn off all notes, sounds and controllers on all
* MIDI channels to the given MidiBuffer. This is useful for stopping
* all sound when the user closes the plugin or changes presets.
*
* \param midi The MidiBuffer to send messages to.
*/
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

//==============================================================================
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

/**
* \brief Sets the onset interval for each player in the ensemble to the current tempo.
*
* This is only done if the tempo has not yet been set. This is a one-time operation
* that is used to set the initial tempo for the players. The tempo is set to the
* current tempo of the processor, which is stored in the samplesPerBeat.
*/
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

/**
 * \brief Checks if all players have played a note and thus if
 * there are new onset times available to be used for the next iteration of
 * the ensemble model.
 *
 * \returns true if all players have played a note, false otherwise
 */
bool EnsembleModel::newOnsetsAvailable()
{
	bool available = true;

	for (auto& player : players)
	{
		available = available && player->hasNotePlayed();
	}

	return available;
}

/**
* \brief Calculates new onset intervals for each player based on the most recent onset times of the other players and the parameters of the ensemble model.
*
* The new onset intervals are calculated by calling recalculateOnsetInterval on each
* player, and then the next note time is calculated by adding the new interval to
* the most recent onset time for each player. This information is then sent to the
* Max/MSP patch through OSC messages. If logging is enabled, the details of the most
* recent onsets are stored in buffers to be logged.
*/
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
void EnsembleModel::clearOnsetsAvailable()
{
	for (auto& player : players)
	{
		player->resetNotePlayed();
	}
}

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

/**
* \brief Store the log information about the latest onset from the given player in the
* logging buffers.
*
* \param bufferIndex The index of the logging buffer to store the data in.
* \param playerIndex The index of the player whose onset details are to be stored.
*/
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

/**
* @brief Get the Latest Alphas object (NOT IMPLEMENTED)
*/
void EnsembleModel::getLatestAlphas()
{
}

//==============================================================================
/**
* \brief Create a Player for each track in the file which has note on events.
*
* The first 'numUserPlayers' will be UserPlayers, and the rest will be
* Players.
*
* \param file The MidiFile from which to create the players
*/
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

void EnsembleModel::ConnectOSCReceiver(int portNumber)
{
	osc->ConnectReceiver(portNumber);
}

void EnsembleModel::SendActionMessage(juce::String message)
{
	osc->SendActionMessage(message);
}

/**
 * @brief Initialises a matrix of alpha and beta parameters base on the total number of players there are
 */
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

/**
 * \brief Processes a block of MIDI data and updates the state of the EnsembleModel.
 *
 * \param inMidi The input MIDI buffer.
 * \param outMidi The output MIDI buffer.
 * \param sampleIndex The sample index at which the block of MIDI data should be processed.
 */
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

/**
 * \brief Resets all players in the ensemble model
 *
 * Initialising intro countdown, score counter and tempo. Also starts loops for logging onset times and polling for
 * new alpha values.
 */
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

/**
 * @brief NOT IMPLEMENTED
 *
 * @param onsets
 * @param delays
 */
void EnsembleModel::postLatestOnsets(const std::vector<int>& onsets, const std::vector<int>& delays)
{
}

//==============================================================================
/**
* \brief Checks if a MIDI sequence contains any note on events.
*
* This method can be used to check if a \ref juce::MidiMessageSequence contains any note on events. If it does, the method returns true, otherwise it returns false.
*
* \param seq The MIDI sequence to check.
*
* \return true if the sequence contains any note on events, false otherwise.
*/
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