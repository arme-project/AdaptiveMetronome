#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EnsembleModel.h"

using namespace std::chrono;
using namespace std::chrono_literals;

//==============================================================================
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

	OSCAutoConnect = true;

	if (OSCAutoConnect)
	{
		osc->ConnectSender();
		osc->ConnectReceiver();
	}
}

EnsembleModel::~EnsembleModel()
{
	poller->Stop();
}

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

//==============================================================================
// OSC Messaging


juce::MidiFile EnsembleModel::GetMidiFile()
{
	return midiFile;
}

// Connection can be established via config file parameter "OSCReceivePort"


bool EnsembleModel::isOscReceiverConnected()
{
	return (currentReceivePort > -1);
}

void EnsembleModel::oscMessageSend(bool test)
{
	if (test) {
		auto oscMessage = juce::OSCMessage("/test");
		if (!OSCSender.send(oscMessage)) {
			DBG("Error: could not send OSC message.");
		}
	}
	else {
		auto oscMessage = juce::OSCMessage("/onsets");
		for (int i = 0; i < 4; i++) {
			auto randomFloat = 5.0f; // randomizer.nextFloat() / (float)20.0 + (float)0.5;
			oscMessage.addArgument(randomFloat);
		}

		if (!OSCSender.send(oscMessage)) {
			DBG("Error: could not send OSC message.");
		}
	}
}

void EnsembleModel::oscMessageReceived(const juce::OSCMessage & message)
{
	juce::OSCAddressPattern oscPattern = message.getAddressPattern();
	juce::String oscAddress = oscPattern.toString();

	if (oscAddress == "/loadConfig") {
		if (message[0].isString()) {
			auto configFilename = message[0].getString();
			auto configFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(configSubfolder).getChildFile(configFilename);

			if (!configFile.existsAsFile()) { return; }

			config->loadConfig(configFile);
		}
	}
	else if (oscAddress == "/reset")
	{
		reset();
	}
	else if (oscAddress == "/setLogname")
	{
		if (message[0].isString())
		{
			juce::String newFileName = message[0].getString();
			if (!newFileName.endsWith(".csv")) {
				newFileName << ".csv";
			}
			logger->SetFilenameOverride(newFileName);
		}
	}
	else if (oscAddress == "/numIntroTones")
	{
		if (message[0].isInt32())
		{
			numIntroTones = message[0].getInt32();
		}
	}
	else if (oscAddress == "/plugin") // New User Note from external (e.g. Max). The third argument is the time per the MaxMSP cpu clock, and is now redundant.
	{
		if (message[0].isFloat32() && message[1].isInt32()
			&& message[2].isInt32() && message[3].isFloat32()) {
			float oscOnsetTime = message[0].getFloat32();
			int onsetNoteNumber = message[1].getInt32();
			int msMax = message[2].getInt32();

			if (processor->manualPlaying) {
				if (waitingForFirstNote && onsetNoteNumber == 0) {
					triggerFirstNote();
					setUserOnsetFromOsc(oscOnsetTime, onsetNoteNumber, msMax);
				}
				else if (onsetNoteNumber > 0) {
					setUserOnsetFromOsc(oscOnsetTime, onsetNoteNumber, msMax);
				}
			}
		}
	}
	else if (oscAddress == "/playbackstart") // Only used to set timer at start of playback. No longer needed.
	{
		if (message[0].isInt32()) {                             // [5]
			if (waitingForFirstNote && processor->manualPlaying) {
				//clock.setStartOfPlayback(message[0].getInt32());
				//DBG("Start playback - " << clock.tickToString(clock.tick()));
			}
		}
	}
	else if (oscAddress == "/oscstart")
	{
		reset(true);
		processor->setManualPlaying(true);
	}

	sendActionMessage("OSC Received");
}

//==============================================================================
bool EnsembleModel::loadMidiFile(const juce::File & file, int userPlayers)
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

void EnsembleModel::triggerFirstNote() {
	waitingForFirstNote = false;
}

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

bool EnsembleModel::reset(bool skipIntroNotes)
{
	reset();
	introTonesPlayed = numIntroTones;

	return false;
}

//==============================================================================
void EnsembleModel::prepareToPlay(double newSampleRate)
{
	sampleRate = newSampleRate;
}

void EnsembleModel::releaseResources()
{
}

//==============================================================================
// Main method for processing incoming midi stream
void EnsembleModel::processMidiBlock(const juce::MidiBuffer & inMidi, juce::MidiBuffer & outMidi, int numSamples, double tempo)
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
// This method is called from the PluginProcessor when a note is played via OSC from Max
// oscOnsetTime is in seconds, msMax is the time of the onset in ms according to Max's cpu clock
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
int EnsembleModel::getNumPlayers()
{
	return static_cast <int> (players.size());
}

int EnsembleModel::getNumUserPlayers()
{
	return static_cast <int> (numUserPlayers);
}

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
void EnsembleModel::soundOffAllChannels(juce::MidiBuffer & midi)
{
	for (int channel = 1; channel <= 16; ++channel)
	{
		midi.addEvent(juce::MidiMessage::allNotesOff(channel), 0);
		midi.addEvent(juce::MidiMessage::allSoundOff(channel), 0);
		midi.addEvent(juce::MidiMessage::allControllersOff(channel), 0);
	}
}

//==============================================================================
void EnsembleModel::playIntroTones(juce::MidiBuffer & midi, int sampleIndex)
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

void EnsembleModel::introToneOn(juce::MidiBuffer & midi, int sampleIndex)
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

void EnsembleModel::introToneOff(juce::MidiBuffer & midi, int sampleIndex)
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

bool EnsembleModel::newOnsetsAvailable()
{
	bool available = true;

	for (auto& player : players)
	{
		available = available && player->hasNotePlayed();
	}

	return available;
}

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
		oscMessageSendNewInterval(i, players[i]->getCurrentNoteIndex() + 1, nextNoteTimeInMS);
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

void EnsembleModel::oscMessageSendNewInterval(int playerNum, int noteNum, int noteTimeInMS) {
	auto oscMessage = juce::OSCMessage("/newInterval");
	oscMessage.addInt32(playerNum);
	oscMessage.addInt32(noteNum);
	oscMessage.addInt32(noteTimeInMS);
	if (!OSCSender.send(oscMessage)) {
		DBG("Error: could not send OSC message.");
	}
}

void EnsembleModel::oscMessageSendReset() {
	auto oscMessage = juce::OSCMessage("/reset");
	if (!OSCSender.send(oscMessage)) {
		DBG("Error: could not send OSC message.");
	}
}

void EnsembleModel::oscMessageSendPlayMax() {
	auto oscMessage = juce::OSCMessage("/playMax");
	if (!OSCSender.send(oscMessage)) {
		DBG("Error: could not send OSC message.");
	}
}

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
EnsembleModel::FlagLock::FlagLock(std::atomic_flag & f)
	: flag(f),
	locked(!flag.test_and_set())
{
}

EnsembleModel::FlagLock::~FlagLock()
{
	flag.clear();
}

//==============================================================================
void EnsembleModel::createPlayers(const juce::MidiFile & file)
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

// Initialise matrix of alpha and beta parameters
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

void EnsembleModel::playUserIntro(const juce::MidiBuffer & inMidi, juce::MidiBuffer & outMidi, int sampleIndex)
{
	for (auto& player : players)
	{
		if (player->isUserOperated())
		{
			player->processIntroSample(inMidi, outMidi, sampleIndex, introToneNoteOther);
		}
	}
}

// Called from EnsembleModel::processMidiBlock
void EnsembleModel::playScore(const juce::MidiBuffer & inMidi, juce::MidiBuffer & outMidi, int sampleIndex)
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
bool EnsembleModel::checkMidiSequenceHasNotes(const juce::MidiMessageSequence * seq)
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