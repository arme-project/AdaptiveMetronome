#include <chrono>
#include "Logger.h"

using namespace std::chrono_literals;

Logger::Logger(int numPlayersIn, float sampleRateIn, const std::vector<bool>& isUserFlags) : 
    numPlayers(numPlayersIn), isUserOperated(isUserFlags), sampleRate(sampleRateIn)
{
	InitialiseBuffer();
}

Logger::~Logger()
{
	Stop();
}

/**
* \brief Starts the logging loop for the ensemble model.
*
* This function starts a logging loop for the ensemble model. It first stops any
* existing logging loop, then initialises the logging buffer and starts a new
* thread to run the logging loop. The logging loop will continue to run until
* the stopLoggerLoop function is called.
*/
void Logger::Start()
{
	Stop(); //Stops any occuring Logging Threads before starting a new one
	continueLogging = true;
	thread = std::thread([this]() {this->loggerLoop(); });
}

/**
 * \brief Stops the logger loop by setting the continueLogging flag to false.
 *
 * If the logger thread is joinable, it will join the thread to ensure proper cleanup.
 */
void Logger::Stop() {
	continueLogging = false;
	if (thread.joinable())
		thread.join();
}


/**
 * \brief Overrides the filename of the logging file 
 * \param filename Name of the file to store it in
 */
void Logger::SetFilenameOverride(juce::String filename)
{
    logFilenameOverride = filename;
}

/**
 * \brief Changes the subfolder where the logging results are stored in
 * \param folderName Name of the directory to store it in
 */
void Logger::SetSubFolder(juce::String folderName)
{
    logSubfolder = folderName;
}

/**
 * \brief Adds an entry for a player to store in the logging file
 * \param entry Data corresponding to a player using the LogData structure
 */
void Logger::AddEntry(const LogData& entry)
{
    auto write = fifo->write(1);
    if (write.blockSize1 > 0)
    {
        loggingBuffer[write.startIndex1] = entry;
        fifo->finishedWrite(write.blockSize1);
    }
}

juce::String Logger::GetFileNameOverride()
{
    return logFilenameOverride;
}

/**
 * \brief Write header line to log file.
 *
 * This includes columns for each player's note onsets, intervals,
 * user input, delay, and noise parameters. It also includes columns for each pair of players'
 * asynchronous and alpha/beta parameters. Finally, it includes columns for each player's velocity.
 *
 * \param logStream The file stream to write the log to.
 */
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


/**
* \brief Initialises the logging buffer.
*
* This is done by creating a AbstractFifo of size 4 * the number of players (or 4 if there are no players),
* and then resizing the logging buffer to the same size. The buffer is then filled with LogEntry objects,
* each with asyncs, alphas and betas of size equal to the number of players (or 0 if there are no players),
* and all elements are set to 0.0.
*/
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

/**
 * \brief Starts a logging loop, which writes details of each tap to a file in the user's documents folder.
 *
 * This function is run in a separate thread, and continues until the user stops the logger.
 * TODO: Add checks to ensure that the file is not already open.
 *
 * The logging loop is started by calling EnsembleModel::startLoggerLoop().
 * It can be stopped by calling EnsembleModel::stopLoggerLoop().
 */
void Logger::loggerLoop()
{
    // Expose this option to UI at some point.
    auto time = juce::Time::getCurrentTime();

    // Start with default documents folder
    juce::File logFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);

    // Add subfolder, if this is specified
    if (logSubfolder != "")
    {
        logFile = logFile.getChildFile (logSubfolder);
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

/**
* Logs the onset details of the ensemble to a file.
*
* This function is called on a separate thread, so as not to block the audio thread.
*
* \param logStream The file stream to write the log to.
*/
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


