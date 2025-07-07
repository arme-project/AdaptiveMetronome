/**
* \file Logger.cpp
* \brief Implementation of the Logger class for logging ensemble model data.
* 
* This file contains the implementation of the Logger class, which is responsible for logging
* the details of the ensemble model, including player onsets, intervals, user inputs,
*/

#include <chrono>
#include "Logger.h"

using namespace std::chrono_literals;

// Constructor for Logger class
Logger::Logger(int numPlayersIn, float sampleRateIn, const std::vector<bool>& isUserFlags) :
	numPlayers(numPlayersIn), isUserOperated(isUserFlags), sampleRate(sampleRateIn)
{
	InitialiseBuffer();
}

// Destructor for Logger class
Logger::~Logger()
{
	Stop();
}

// Starts the logger loop by stopping any existing logging thread, initialising the buffer, and starting a new thread.
void Logger::Start()
{
	Stop(); //Stops any occuring Logging Threads before starting a new one
	InitialiseBuffer(); // Initialises the logging buffer
	continueLogging = true;
	thread = std::thread([this]() {this->loggerLoop(); });
}

void Logger::Start(int numPlayersIn)
{
	numPlayers = numPlayersIn;
	Start();
}

// Stops the logger loop by setting the continueLogging flag to false and joining the thread if it is joinable.
void Logger::Stop() {
	continueLogging = false;
	if (thread.joinable())
		thread.join();
}

// Sets the subfolder where the logging results will be stored.
void Logger::SetSubFolder(juce::String folderName)
{
	logSubfolder = folderName;
}

// AddEntry function adds a new log entry to the logging buffer.
void Logger::AddEntry(const LogData& entry)
{
	auto write = fifo->write(1);
	if (write.blockSize1 > 0)
	{
		loggingBuffer[write.startIndex1] = entry;
		fifo->finishedWrite(write.blockSize1);
	}
}

// Overrides the filename of the logging file
void Logger::SetFilenameOverride(juce::String filename)
{
	logFilenameOverride = filename;
}

// Returns the filename override for the logging file.
juce::String Logger::GetFileNameOverride()
{
	return logFilenameOverride;
}

 // WriteHeaders function writes the header line to the log file
void Logger::WriteHeaders(juce::FileOutputStream& logStream)
{
	juce::String logLine("N");
	juce::String onsetLog, intervalLog, userInputLog, delayLog,
		mNoiseLog, tkNoiseLog, asyncLog, alphaLog, betaLog,
		tkNoiseStdLog, mNoiseStdLog, velocityLog;

	for (int i = 0; i < numPlayers; ++i)
	{
		int playerId = i + 1;

		onsetLog += ", P" + juce::String(playerId) + (isUserOperated[i] ? " (input)" : "");
		intervalLog += ", P" + juce::String(playerId) + " Int";
		userInputLog += ", P" + juce::String(playerId) + " User Input";
		delayLog += ", P" + juce::String(playerId) + " Delay";
		mNoiseLog += ", P" + juce::String(playerId) + " MVar";
		tkNoiseLog += ", P" + juce::String(playerId) + " TKVar";

		for (int j = 0; j < numPlayers; ++j)
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

// A bunch of stuff for safely logging onset times and sending them out to the
// server. Functions defined in here are only safe to call from the logging thread.

// Initialises the logging buffer with a size based on the number of players.
void Logger::InitialiseBuffer()
{
	// Define the buffer size to be either 4 (if number of players is 0) or 4 x number of players
	int bufferSize = std::max(4 * numPlayers, 4);

	fifo = std::make_unique <juce::AbstractFifo>(bufferSize);
	loggingBuffer.resize(bufferSize);

	// Resize the buffer so that it corresponds for each player correctly
	for (int i = 0; i < loggingBuffer.size(); ++i) {
		loggingBuffer[i].asyncs.resize(numPlayers, 0.0);
		loggingBuffer[i].alphas.resize(numPlayers, 0.0);
		loggingBuffer[i].betas.resize(numPlayers, 0.0);
	}
}

// Called by the thread to start the logging loop to write details of each tap to a file in the user's documents folder.
void Logger::loggerLoop()
{
	// Expose this option to UI at some point.
	auto time = juce::Time::getCurrentTime();

	// Start with default documents folder
	juce::File logFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);

	// Add subfolder, if this is specified
	if (logSubfolder != "")
	{
		logFile = logFile.getChildFile(logSubfolder);
		if (!logFile.exists()) {
			logFile.createDirectory();
		}
	}

	// Check if the log filename has also been overriden via config.
	if (logFilenameOverride != "") {
		// TODO What happens if overriden log file already exists? Override? Create a new one with slightly different name?
		logFile = logFile.getChildFile(logFilenameOverride).getNonexistentSibling();
	}
	else {
		auto logFileName = time.formatted("Log_%H-%M-%S_%d%b%Y.csv");
		logFile = logFile.getChildFile(logFileName);
	}

	juce::FileOutputStream logStream(logFile);
	logStream.setPosition(0);
	logStream.truncate();

	// Write log file
	WriteHeaders(logStream);

	lineCounter = 0;

	while (continueLogging)
	{
		logOnsetDetails(logStream);
		std::this_thread::sleep_for(50ms);
	}
}

// Logs the onset details of the ensemble to a file.
void Logger::logOnsetDetails(juce::FileOutputStream& stream)
{
	while (fifo->getNumReady() > 0)
	{
		std::vector <int> latestOnsets(numPlayers), latestDelays(numPlayers);
		juce::String logLine(lineCounter++);
		juce::String onsetLog, intervalLog, userInputLog, delayLog,
			mNoiseLog, tkNoiseLog, asyncLog, alphaLog, betaLog,
			tkNoiseStdLog, mNoiseStdLog, velocityLog;

		int p = 0;

		auto reader = fifo->read(static_cast <int> (numPlayers));

		for (int i = 0; i < reader.blockSize1; ++i)
		{
			// Append to array to send to server.
			int bufferIndex = reader.startIndex1 + i;

			auto& data = loggingBuffer[bufferIndex];
			latestOnsets[p] = data.onsetTime;
			latestDelays[p] = data.delay;
			++p;

			// Log to log file
			logOnsetDetailsForPlayer(bufferIndex,
				onsetLog,
				intervalLog,
				userInputLog,
				delayLog,
				mNoiseLog,
				tkNoiseLog,
				asyncLog,
				alphaLog,
				betaLog,
				tkNoiseStdLog,
				mNoiseStdLog,
				velocityLog);
		}

		for (int i = 0; i < reader.blockSize2; ++i)
		{
			// Append to array to send to server.
			int bufferIndex = reader.startIndex2 + i;

			auto& data = loggingBuffer[bufferIndex];
			latestOnsets[p] = data.onsetTime;
			latestDelays[p] = data.delay;
			++p;

			// Log to log file
			logOnsetDetailsForPlayer(bufferIndex,
				onsetLog,
				intervalLog,
				userInputLog,
				delayLog,
				mNoiseLog,
				tkNoiseLog,
				asyncLog,
				alphaLog,
				betaLog,
				tkNoiseStdLog,
				mNoiseStdLog,
				velocityLog);
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

		stream.writeText(logLine, false, false, nullptr);

		// Send onset detail to wherever they need to go.
		//postLatestOnsets(latestOnsets, latestDelays);
	}
}

/**
* \brief Logs onset details for a single player to strings which will be written to file.
*
* \param bufferIndex The index of the logging buffer containing the onset details.
* \param onsetLog The string to which the onset time will be appended.
* \param intervalLog The string to which the onset interval will be appended.
* \param userInputLog The string to which the "user input" flag will be appended.
* \param delayLog The string to which the delay will be appended.
* \param mNoiseLog The string to which the motor noise will be appended.
* \param tkNoiseLog The string to which the timekeeper noise will be appended.
* \param asyncLog The string to which the asynchronies will be appended.
* \param alphaLog The string to which the alphas will be appended.
* \param betaLog The string to which the betas will be appended.
* \param tkNoiseStdLog The string to which the timekeeper noise standard deviation will be appended.
* \param mNoiseStdLog The string to which the motor noise standard deviation will be appended.
* \param velocityLog The string to which the velocity will be appended.
*/
void Logger::logOnsetDetailsForPlayer(int bufferIndex,
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