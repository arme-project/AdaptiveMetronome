#include "JuceHeader.h"
#include "PluginProcessor.h"
#include "EnsembleModel.h"
#include "UserPlayer.h"
#include "OSCManager.h"

using namespace std::chrono_literals;

class OSCManager;

//==============================================================================

#pragma region Constructor and Deconstructor

EnsembleModel::EnsembleModel(AdaptiveMetronomeAudioProcessor* processorPtr)
	: processor(processorPtr)
{

	oscManager = std::make_unique<OSCManager>(this);
	xmlManager = std::make_unique<XMLManager>(this);

	playersInUse.clear();
	resetFlag.clear();

	oscManager->addOSCAddresses();
}

EnsembleModel::~EnsembleModel()
{
	stopLoggerLoop();
	stopPollingLoop();
}
#pragma endregion

//==============================================================================

#pragma region Getters and Setters

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

juce::AudioParameterFloat& EnsembleModel::getPlayerDelayParameter(int playerIndex)
{
	return *processor->delayParameter(playerIndex);
}

juce::AudioParameterFloat& EnsembleModel::getPlayerMotorNoiseParameter(int playerIndex)
{
	return *processor->mNoiseStdParameter(playerIndex);
}

juce::AudioParameterFloat& EnsembleModel::getPlayerTimeKeeperNoiseParameter(int playerIndex)
{
	return *processor->tkNoiseStdParameter(playerIndex);
}

juce::AudioParameterFloat& EnsembleModel::getPlayerVolumeParameter(int playerIndex)
{
	return *processor->volumeParameter(playerIndex);
}

juce::AudioParameterFloat& EnsembleModel::getAlphaParameter(int player1Index, int player2Index)
{
	return *processor->alphaParameter(player1Index, player2Index);
}

juce::AudioParameterFloat& EnsembleModel::getBetaParameter(int player1Index, int player2Index)
{
	return *processor->betaParameter(player1Index, player2Index);
}

#pragma endregion

//==============================================================================

#pragma region Helpful Functions

// Sets the alpha parameters for all players in the processor to a specified value
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

// Prepares the model for playback by setting the new sample rate
void EnsembleModel::prepareToPlay(double newSampleRate)
{
	sampleRate = newSampleRate;
}

// Releases resources used by the model
void EnsembleModel::releaseResources()
{
	return;
}

// Resets the state of all players and re-initialises variables, such as counters and loops for logging and polling
void EnsembleModel::resetPlayers()
{
	// Initialize intro countdown and set intro tones played to 0
	introCounter = 0;
	introTonesPlayed = 0;

	// Reset score counter
	scoreCounter = 0;

	// Ensure tempo will be set on playback start
	initialTempoSet = false;

	// Start logger loop for recording onset times of each player
	startLoggerLoop();

	// Start polling loop to fetch new alpha values
	startPollingLoop();

	// Reset each player in the players array
	for (auto& player : players)
	{
		player->reset();
	}

	// Clear reset flag to indicate players have been reset
	resetFlag.clear();
}

// Initialises the alpha and beta parameters for each player in the matrix, setting default values
void EnsembleModel::createAlphaBetaParameters()
{
	for (int i = 0; i < players.size(); ++i)
	{
		double alpha = 0.25;  // Default alpha value
		double beta = 0.1;    // Default beta value

		for (int j = 0; j < players.size(); ++j)
		{
			*processor->alphaParameter(i, j) = alpha;
			*processor->betaParameter(i, j) = beta;
		}
	}
}

#pragma endregion Functions mainly related to Alpha and Betas, GUI, and others

//==============================================================================

#pragma region OSC Functions

// Establishes a connection to an OSC sender at the specified IP address and port number.
// Default IP is "127.0.0.1" and the default port is 8000.
void EnsembleModel::connectOSCSender(int portNumber=8000, juce::String IPAddress = "127.0.0.1")
{
	oscManager->connectOSCSender(portNumber, IPAddress);
}

// Establishes a connection to an OSC receiver at the specified port number.
// If the connection is successful, the current receive port is updated, otherwise an error is logged.
void EnsembleModel::connectOSCReceiver(int portNumber)
{
	oscManager->connectOSCReceiver(portNumber);
}

// Checks if the OSC receiver is currently connected by verifying if the port number is valid.
bool EnsembleModel::isOscReceiverConnected()
{
	return oscManager->isOscReceiverConnected();
}

// Handles the retrieval of OSC messages.
void EnsembleModel::oscMessageReceived(const juce::OSCMessage& message)
{
	oscManager->oscMessageReceived(message);
}

int EnsembleModel::getCurrentReceivePort() {
	return oscManager->getCurrentReceivePort();
}
#pragma endregion OSC related functions that interact with the OSCManager Class object

//==============================================================================

#pragma region XML Functions

// XML CONFIG FUNCTIONS
// Loading requires converting: xml file -> xmlDocument -> xmlElement

// loadConfigFromXml can be called directly with XmlElement ... or from a File via parseXmlConfigFileToXmlElement
void EnsembleModel::loadConfigFromXml(juce::File configFile) {
	xmlManager->loadConfig(configFile);
}

// Formats the current ensemble state to xml, and saves it to a file (currently a default file in user folder)
// Note: This currently only saves alpha and beta parameters.
void EnsembleModel::saveConfigToXmlFile() {
	xmlManager->saveConfig();
}

#pragma endregion XML related functions that interacts with the (possible) XMLManager Class object

//==============================================================================

#pragma region Logging Functions

// Initializes the logging buffer based on the number of players.
// Sets the size of the buffer, creates the FIFO (First In, First Out) structure, and initializes the async, alpha, and beta parameters for logging.
	void EnsembleModel::initialiseLoggingBuffer()
	{
		int bufferSize = 0;
		if (players.size() > 0)
		{
			bufferSize = static_cast<int>(4 * players.size());
		}
		else {
			bufferSize = 4;
		}

		loggingFifo = std::make_unique<juce::AbstractFifo>(bufferSize);
		loggingBuffer.resize(bufferSize);

		for (int i = 0; i < loggingBuffer.size(); ++i)
		{
			loggingBuffer[i].asyncs.resize(players.size(), 0.0);
			loggingBuffer[i].alphas.resize(players.size(), 0.0);
			loggingBuffer[i].betas.resize(players.size(), 0.0);
		}
	}

	// Starts the logger loop in a separate thread, initializing the logging buffer and setting a flag to continue logging.
	void EnsembleModel::startLoggerLoop()
	{
		stopLoggerLoop();
		initialiseLoggingBuffer();

		continueLogging = true;
		loggerThread = std::thread([this]() { this->loggerLoop(); });
	}

	// Stops the logger loop by setting the flag to false and joining the logging thread.
	void EnsembleModel::stopLoggerLoop()
	{
		continueLogging = false;

		if (loggerThread.joinable())
		{
			loggerThread.join();
		}
	}

	// Main logging loop that writes log data to a file at regular intervals.
	// Creates a log file in the user's documents folder (with optional subfolder and file name overrides), writes a header, and logs onset details.
	void EnsembleModel::loggerLoop()
	{
		// Get the current time to use in log file naming.
		auto time = juce::Time::getCurrentTime();

		// Start with default documents folder.
		juce::File logFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);

		// Add subfolder if specified.
		if (logSubfolder != "")
		{
			logFile = logFile.getChildFile(logSubfolder);
			if (!logFile.exists()) {
				logFile.createDirectory();
			}
		}

		// Check if the log filename has been overridden via config.
		if (logFilenameOverride != "")
		{
			logFile = logFile.getChildFile(logFilenameOverride).getNonexistentSibling();
		}
		else {
			auto logFileName = time.formatted("Log_%H-%M-%S_%d%b%Y.csv");
			logFile = logFile.getChildFile(logFileName);
		}

		// Open the log file for writing.
		juce::FileOutputStream logStream(logFile);
		logStream.setPosition(0);
		logStream.truncate();

		// Write the header for the log file.
		writeLogHeader(logStream);

		logLineCounter = 0;

		// Main logging loop, logs onset details every 50ms.
		while (continueLogging)
		{
			logOnsetDetails(logStream);
			std::this_thread::sleep_for(50ms);
		}
	}

	// Writes the header (column names) for the log file, including player information, delays, noise, asyncs, alphas, betas, etc.
	void EnsembleModel::writeLogHeader(juce::FileOutputStream& logStream)
	{
		juce::String logLine("N");
		juce::String onsetLog, intervalLog, userInputLog, delayLog,
			mNoiseLog, tkNoiseLog, asyncLog, alphaLog, betaLog,
			tkNoiseStdLog, mNoiseStdLog, velocityLog;

		for (int i = 0; i < players.size(); ++i)
		{
			int playerId = i + 1;

			onsetLog += ", P" + juce::String(playerId) + (players[i]->isUserOperated() ? " (input)" : "");
			intervalLog += ", P" + juce::String(playerId) + " Int";
			userInputLog += ", P" + juce::String(playerId) + " User Input";
			delayLog += ", P" + juce::String(playerId) + " Delay";
			mNoiseLog += ", P" + juce::String(playerId) + " MVar";
			tkNoiseLog += ", P" + juce::String(playerId) + " TKVar";

			for (int j = 0; j < players.size(); ++j)
			{
				int otherPlayerId = j + 1;

				asyncLog += ", Async " + juce::String(playerId) + juce::String(otherPlayerId);
				alphaLog += ", Alpha " + juce::String(playerId) + juce::String(otherPlayerId);
				betaLog += ", Beta " + juce::String(playerId) + juce::String(otherPlayerId);
			}

			tkNoiseStdLog += ", P" + juce::String(playerId) + " TKStd";
			mNoiseStdLog += ", P" + juce::String(playerId) + " MStd";
			velocityLog += ", P" + juce::String(playerId) + " Vol";
		}

		logLine += onsetLog + ", " +
			intervalLog + ", " +
			userInputLog + ", " +
			delayLog + ", " +
			mNoiseLog + ", " +
			tkNoiseLog + ", " +
			asyncLog + ", " +
			alphaLog + ", " +
			betaLog + ", " +
			tkNoiseStdLog + ", " +
			mNoiseStdLog + ", " +
			velocityLog + "\n";

		logStream.writeText(logLine, false, false, nullptr);
	}

	// Logs onset details for each player and writes them to the log file.
	// This function reads from the logging FIFO, writes onset data for all players, and sends the latest onsets to a server.
	void EnsembleModel::logOnsetDetails(juce::FileOutputStream& logStream)
	{
		while (loggingFifo->getNumReady() > 0)
		{
			std::vector<int> latestOnsets(players.size()), latestDelays(players.size());
			juce::String logLine(logLineCounter++);
			juce::String onsetLog, intervalLog, userInputLog, delayLog,
				mNoiseLog, tkNoiseLog, asyncLog, alphaLog, betaLog,
				tkNoiseStdLog, mNoiseStdLog, velocityLog;

			int p = 0;

			auto reader = loggingFifo->read(static_cast<int>(players.size()));

			for (int i = 0; i < reader.blockSize1; ++i)
			{
				int bufferIndex = reader.startIndex1 + i;
				auto& data = loggingBuffer[bufferIndex];
				latestOnsets[p] = data.onsetTime;
				latestDelays[p] = data.delay;
				++p;

				logOnsetDetailsForPlayer(bufferIndex, onsetLog, intervalLog, userInputLog, delayLog, mNoiseLog, tkNoiseLog, asyncLog, alphaLog, betaLog, tkNoiseStdLog, mNoiseStdLog, velocityLog);
			}

			for (int i = 0; i < reader.blockSize2; ++i)
			{
				int bufferIndex = reader.startIndex2 + i;
				auto& data = loggingBuffer[bufferIndex];
				latestOnsets[p] = data.onsetTime;
				latestDelays[p] = data.delay;
				++p;

				logOnsetDetailsForPlayer(bufferIndex, onsetLog, intervalLog, userInputLog, delayLog, mNoiseLog, tkNoiseLog, asyncLog, alphaLog, betaLog, tkNoiseStdLog, mNoiseStdLog, velocityLog);
			}

			logLine += onsetLog + ", " +
				intervalLog + ", " +
				userInputLog + ", " +
				delayLog + ", " +
				mNoiseLog + ", " +
				tkNoiseLog + ", " +
				asyncLog + ", " +
				alphaLog + ", " +
				betaLog + ", " +
				tkNoiseStdLog + ", " +
				mNoiseStdLog + ", " +
				velocityLog + "\n";

			logStream.writeText(logLine, false, false, nullptr);

			// Send onset detail to wherever they need to go.
			postLatestOnsets(latestOnsets, latestDelays);
		}
	}

	// Logs onset details for a specific player based on the buffer index, filling the relevant log strings with data.
	void EnsembleModel::logOnsetDetailsForPlayer(int bufferIndex,
		juce::String& onsetLog,
		juce::String& intervalLog,
		juce::String& userInputLog,
		juce::String& delayLog,
		juce::String& mNoiseLog,
		juce::String& tkNoiseLog,
		juce::String& asyncLog,
		juce::String& alphaLog,
		juce::String& betaLog,
		juce::String& tkNoiseStdLog,
		juce::String& mNoiseStdLog,
		juce::String& velocityLog)
	{
		auto& data = loggingBuffer[bufferIndex];

		onsetLog += ", " + juce::String(data.onsetTime / sampleRate);
		intervalLog += ", " + juce::String(data.onsetInterval / sampleRate);
		userInputLog += ", " + juce::String(data.userInput ? "true" : "false");
		delayLog += ", " + juce::String(data.delay / sampleRate);
		mNoiseLog += ", " + juce::String(data.motorNoise);
		tkNoiseLog += ", " + juce::String(data.timeKeeperNoise);

		for (int i = 0; i < data.asyncs.size(); ++i)
		{
			asyncLog += "," + juce::String(data.asyncs[i] / sampleRate);
			alphaLog += "," + juce::String(data.alphas[i]);
			betaLog += "," + juce::String(data.betas[i]);
		}

		tkNoiseStdLog += ", " + juce::String(data.tkNoiseStd);
		mNoiseStdLog += ", " + juce::String(data.mNoiseStd);
		velocityLog += ", " + juce::String(data.volume);
	}

	// NOT IMPLEMENTED
	void EnsembleModel::postLatestOnsets(const std::vector <int>& onsets, const std::vector <int>& delays)
	{
		//==========================================================================
		// onsets contains the onset time in samples for each of the players' most
		// recently played notes.
		//
		// delays contains the delays for each player in samples
		//
		// Send these to the server however you want here. The current sampling
		// frequency is available in the sampleRate variable.

		// Once you've sent those to the server indicate to the polling thread that
		// it should start to poll for new alphas.
		alphasUpToDate.clear();
	}

#pragma endregion Logging related functions that interacts with the (possible) LoggingManager Class object

//==============================================================================

#pragma region MIDI Functions

bool EnsembleModel::loadMidiFile(const juce::File & file, int userPlayers)
{
	if (FlagLock lock(playersInUse); !lock.locked)
	{
		return false;
	}

	//==========================================================================
	// Read in content of MIDI file.
	juce::FileInputStream inStream(file);

	if (!inStream.openedOk())
		return false; // put some error handling here

	int fileType = 0;

	if (!midiFile.readFrom(inStream, true, &fileType)) {
		return false; // more error handling
	}

	midiFile.convertTimestampTicksToSeconds();

	//==========================================================================
	// Create player for each track in the file.
	numUserPlayers = userPlayers;
	createPlayers(midiFile); // create new players
	resetPlayers();

	return true;
}

// Main method for processing incoming midi stream
void EnsembleModel::processMidiBlock(const juce::MidiBuffer & inMidi, juce::MidiBuffer & outMidi, int numSamples, double tempo)
{
	if (FlagLock lock(playersInUse); !lock.locked)
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

void EnsembleModel::soundOffAllChannels(juce::MidiBuffer & midi)
{
	for (int channel = 1; channel <= 16; ++channel)
	{
		midi.addEvent(juce::MidiMessage::allNotesOff(channel), 0);
		midi.addEvent(juce::MidiMessage::allSoundOff(channel), 0);
		midi.addEvent(juce::MidiMessage::allControllersOff(channel), 0);
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
			//            players [i]->recalculateOnsetInterval (samplesPerBeat, players, (*alphaParams) [i], (*betaParams) [i]);
			players[i]->recalculateOnsetInterval(samplesPerBeat, players);
		}
	}

	//==========================================================================
	// Add details of most recent onsets to buffers to be logged.
	if (loggingFifo)
	{
		auto writer = loggingFifo->write(static_cast <int> (players.size()));

		int p = 0;

		for (int i = 0; i < writer.blockSize1; ++i)
		{
			storeOnsetDetailsForPlayer(writer.startIndex1 + i, p++);
		}

		for (int i = 0; i < writer.blockSize2; ++i)
		{
			storeOnsetDetailsForPlayer(writer.startIndex2 + i, p++);
		}
	}
}

void EnsembleModel::clearOnsetsAvailable()
{
	for (auto& player : players)
	{
		player->resetNotePlayed();
	}
}

void EnsembleModel::getLatestAlphas()
{
	//    if (pollingFifo)
	//    {
	//        // Consume everything in the buffer, only using the most recent set of alphas.
	//        auto reader = pollingFifo->read (pollingFifo->getNumReady());
	//
	//        for (int player1 = 0; player1 < pollingBuffer.size(); ++player1)
	//        {
	//            int player2 = 0;
	//
	//            int block1Start = std::max (reader.blockSize1 + reader.blockSize2 - static_cast <int> (players.size()), 0);
	//
	//            for (int i = block1Start; i < reader.blockSize1; ++i)
	//            {
	//                *(*alphaParams) [player1][player2++] = pollingBuffer [player1][reader.startIndex1 + i];
	//            }
	//
	//            int block2Start = std::max (block1Start - reader.blockSize1, 0);
	//
	//            for (int i = block2Start; i < reader.blockSize2; ++i)
	//            {
	//                *(*alphaParams) [player1][player2++] = pollingBuffer [player1][reader.startIndex2 + i];
	//            }
	//        }
	//    }
}

void EnsembleModel::storeOnsetDetailsForPlayer(int bufferIndex, int playerIndex)
{
	// Store the log information about the latest onset from the given player
	// in the logging buffers.
	auto& data = loggingBuffer[bufferIndex];

	data.onsetTime = players[playerIndex]->getLatestOnsetTime();
	data.onsetInterval = players[playerIndex]->getPlayedOnsetInterval();
	data.userInput = players[playerIndex]->wasLatestOnsetUserInput();
	data.delay = players[playerIndex]->getLatestOnsetDelay();
	data.motorNoise = players[playerIndex]->getMotorNoise();
	data.timeKeeperNoise = players[playerIndex]->getTimeKeeperNoise();

	for (int i = 0; i < players.size(); ++i)
	{
		data.asyncs[i] = players[playerIndex]->getLatestOnsetTime() - players[i]->getLatestOnsetTime();
		//        data.alphas [i] = *(*alphaParams) [playerIndex][i];
		data.alphas[i] = processor->alphaParameter(playerIndex, i)->get();

		//        data.betas [i] = *(*betaParams) [playerIndex][i];
		data.betas[i] = processor->betaParameter(playerIndex, i)->get();
	}

	data.tkNoiseStd = players[playerIndex]->getTimeKeeperNoiseStd();
	data.mNoiseStd = players[playerIndex]->getMotorNoiseStd();
	data.volume = players[playerIndex]->getLatestVolume();
}

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
	createAlphaBetaParameters(); // create matrix of parameters for alphas
}

#pragma endregion Functions related to MIDI processing (MOST FUNCTIONS ARE HERE)

//==============================================================================

#pragma region Helpful Functions

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

#pragma endregion

//==============================================================================

#pragma region IntroTone Functions

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

#pragma endregion

//==============================================================================

#pragma region FlagLock Functions

EnsembleModel::FlagLock::FlagLock(std::atomic_flag & f)
	: flag(f),
	locked(!flag.test_and_set())
{
}

EnsembleModel::FlagLock::~FlagLock()
{
	flag.clear();
}

#pragma endregion

//==============================================================================

#pragma region Polling Functions

void EnsembleModel::initialisePollingBuffers()
{
	int bufferSize = static_cast <int> (10 * players.size());

	pollingFifo = std::make_unique <juce::AbstractFifo>(bufferSize);

	pollingBuffer.resize(players.size());

	for (int i = 0; i < pollingBuffer.size(); ++i)
	{
		pollingBuffer[i].resize(bufferSize, 0.0);
	}
}

void EnsembleModel::startPollingLoop()
{
	stopPollingLoop();
	initialisePollingBuffers();

	continuePolling = true;
	alphasUpToDate.test_and_set();
	pollingThread = std::thread([this]() {this->pollingLoop(); });
}

void EnsembleModel::stopPollingLoop()
{
	continuePolling = false;

	if (pollingThread.joinable())
	{
		pollingThread.join();
	}
}

void EnsembleModel::pollingLoop()
{
	while (continuePolling)
	{
		if (!alphasUpToDate.test_and_set())
		{
			// getNewAlphas(); //Not used
		}

		std::this_thread::sleep_for(50ms);
	}
}

#pragma endregion

//==============================================================================

//// NOT USED
	//void EnsembleModel::getNewAlphas()
	//{
	//    //==========================================================================
	//    // In here you should make a request to your server to ask for new alpha
	//    // values. If you get some updated values set the following value to true.
	//    // If not, set the value to false and the plug-in will poll again after
	//    // short time.
	//    bool newAlphas = false;
	//
	//    if (newAlphas)
	//    {
	//        auto writer = pollingFifo->write (static_cast <int> (players.size()));
	//
	//        for (int player1 = 0; player1 < pollingBuffer.size(); ++player1)
	//        {
	//            int player2 = 0;
	//
	//            for (int i = 0; i < writer.blockSize1; ++i)
	//            {
	//                // Replace the 0.2 with the alpha parameter for player1_player2
	//                pollingBuffer [player1][writer.startIndex1 + i] = 0.2;
	//                ++player2;
	//            }
	//
	//            for (int i = 0; i < writer.blockSize2; ++i)
	//            {
	//                // Replace the 0.2 with the alpha parameter for player1_player2
	//                pollingBuffer [player1][writer.startIndex2 + i] = 0.2;
	//                ++player2;
	//            }
	//        }
	//    }
	//    else
	//    {
	//        alphasUpToDate.clear();
	//    }
	//}

