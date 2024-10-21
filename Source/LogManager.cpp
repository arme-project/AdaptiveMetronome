/*
  ==============================================================================

    LogManager.cpp
    Created: 21 Oct 2024 12:04:41am
    Author:  jhund

  ==============================================================================
*/

#include "LogManager.h"
#include <thread>

using std::function;
using namespace std::chrono_literals;

LogManager::LogManager(EnsembleModel* model)
    : ensembleModel(model) {}

LogManager::~LogManager() = default;

void LogManager::initialiseLoggingBuffer()
{
    int playerSize = ensembleModel->players.size();
    int bufferSize = playerSize > 0 ? static_cast<int>(4 * playerSize) : 4;

    loggingFifo = std::make_unique<juce::AbstractFifo>(bufferSize);
    loggingBuffer.resize(bufferSize);

    for (int i = 0; i < loggingBuffer.size(); i++) {
        loggingBuffer[i].asyncs.resize(playerSize, 0.0);
        loggingBuffer[i].alphas.resize(playerSize, 0.0);
        loggingBuffer[i].betas.resize(playerSize, 0.0);
    }

}

void LogManager::startLoggerLoop() {
    stopLoggerLoop(); // Stops any existing loops that might be occuring
    initialiseLoggingBuffer();
	
    continueLogging = true;
    loggerThread = std::thread([this]() { this->loggerLoop(); });
}

void LogManager::stopLoggerLoop()
{
    continueLogging = false;

    if (loggerThread.joinable())
    {
        loggerThread.join();
    }
}

void LogManager::writeLogHeader(juce::FileOutputStream& logStream)
{
	juce::String logLine("N");
	juce::String onsetLog, intervalLog, userInputLog, delayLog,
		mNoiseLog, tkNoiseLog, asyncLog, alphaLog, betaLog,
		tkNoiseStdLog, mNoiseStdLog, velocityLog;

	int playerSize = ensembleModel->players.size();
	for (int i = 0; i < playerSize; ++i)
	{
		int playerId = i + 1;

		onsetLog += ", P" + juce::String(playerId) + (ensembleModel->players[i]->isUserOperated() ? " (input)" : "");
		intervalLog += ", P" + juce::String(playerId) + " Int";
		userInputLog += ", P" + juce::String(playerId) + " User Input";
		delayLog += ", P" + juce::String(playerId) + " Delay";
		mNoiseLog += ", P" + juce::String(playerId) + " MVar";
		tkNoiseLog += ", P" + juce::String(playerId) + " TKVar";

		for (int j = 0; j < playerSize; ++j)
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

void LogManager::logOnsetDetails(juce::FileOutputStream& logStream)
{
	while (loggingFifo->getNumReady() > 0) {
		int playerSize = ensembleModel->players.size();

		std::vector<int> latestOnsets(playerSize);
		std::vector<int> latestDelays(playerSize);

		juce::String logLine(ensembleModel->logLineCounter++);

		juce::String onsetLog;
		juce::String intervalLog;
		juce::String userInputLog;
		juce::String delayLog;
		juce::String mNoiseLog;
		juce::String tkNoiseLog;
		juce::String asyncLog;
		juce::String alphaLog;
		juce::String betaLog;
		juce::String tkNoiseStdLog;
		juce::String mNoiseStdLog;
		juce::String velocityLog;

		int p = 0;

		auto reader = loggingFifo->read(static_cast<int>(playerSize));

		for (int i = 0; i < reader.blockSize1; ++i)
		{
			int bufferIndex = reader.startIndex1 + i;
			auto const& data = loggingBuffer[bufferIndex];
			latestOnsets[p] = data.onsetTime;
			latestDelays[p] = data.delay;
			++p;

			logOnsetDetailsForPlayer(bufferIndex, onsetLog, intervalLog, userInputLog, delayLog, mNoiseLog, tkNoiseLog, asyncLog, alphaLog, betaLog, tkNoiseStdLog, mNoiseStdLog, velocityLog);
		}

		for (int i = 0; i < reader.blockSize2; ++i)
		{
			int bufferIndex = reader.startIndex2 + i;
			auto const& data = loggingBuffer[bufferIndex];
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

void LogManager::logOnsetDetailsForPlayer(int bufferIndex, juce::String& onsetLog, juce::String& intervalLog, juce::String& userInputLog, juce::String& delayLog, juce::String& mNoiseLog, juce::String& tkNoiseLog, juce::String& asyncLog, juce::String& alphaLog, juce::String& betaLog, juce::String& tkNoiseStdLog, juce::String& mNoiseStdLog, juce::String& velocityLog)
{
	auto& data = loggingBuffer[bufferIndex];
	double sampleRate = ensembleModel->sampleRate;
	
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

void LogManager::postLatestOnsets(const std::vector<int>& onsets, const std::vector<int>& delays) {
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

	ensembleModel->alphasUpToDate.clear();
}

void LogManager::loggerLoop() {
	// Get the current time to use in log file naming.
	auto time = juce::Time::getCurrentTime();

	// Start with default documents folder.
	juce::File logFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);

	// Add subfolder if specified.
	if (ensembleModel->logSubfolder != "")
	{
		logFile = logFile.getChildFile(ensembleModel->logSubfolder);
		if (!logFile.exists()) {
			logFile.createDirectory();
		}
	}

	// Check if the log filename has been overridden via config.
	if (ensembleModel->logFilenameOverride != "")
	{
		logFile = logFile.getChildFile(ensembleModel->logFilenameOverride).getNonexistentSibling();
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

	ensembleModel->logLineCounter = 0;

	// Main logging loop, logs onset details every 50ms.
	while (continueLogging)
	{
		logOnsetDetails(logStream);
		std::this_thread::sleep_for(50ms);
	}
}

