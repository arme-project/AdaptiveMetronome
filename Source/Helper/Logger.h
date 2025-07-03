#pragma once

#include <JuceHeader.h>
#include <thread>
#include <atomic>

/**
 * \class Logger
 * \brief Handles logging of ensemble model data.
 *
 * This class is responsible for logging the onset details of the ensemble model to a file.
 * It runs in a separate thread to avoid blocking the audio thread, and uses an AbstractFifo
 * to manage the logging buffer.
 */
class Logger
{
public:

	/**
	 * \brief A structure to hold the data for each log entry.
	 */
	struct LogData
	{
		int onsetTime = 0;
		int onsetInterval = 0;
		bool userInput = false;
		double delay = 0.0;
		double motorNoise = 0.0;
		double timeKeeperNoise = 0.0;
		std::vector<int> asyncs;
		std::vector<float> alphas;
		std::vector<float> betas;
		double tkNoiseStd = 0.0;
		double mNoiseStd = 0.0;
		double volume = 0.0;
	};

	// Constructor and Deconstructor
	Logger(int numPlayersIn, float sampleRate, const std::vector<bool>& isUserFlags);
	~Logger();

	/**
	* \brief Starts the logging loop for the ensemble model.
	*
	* This function starts a logging loop for the ensemble model. It first stops any
	* existing logging loop, then initialises the logging buffer and starts a new
	* thread to run the logging loop. The logging loop will continue to run until
	* the stopLoggerLoop function is called.
	*/
	void Start();

	/**
	 * \brief Stops the logger loop by setting the continueLogging flag to false.
	 *
	 * If the logger thread is joinable, it will join the thread to ensure proper cleanup.
	 */
	void Stop();

	/**
	 * \brief Overrides the filename of the logging file
	 * \param filename Name of the file to store it in
	 */
	void SetFilenameOverride(juce::String filename);

	/**
	 * \brief Changes the subfolder where the logging results are stored in
	 * \param folderName Name of the directory to store it in
	 */
	void SetSubFolder(juce::String filename);

	/**
	 * \brief Adds an entry for a player to store in the logging file
	 * \param entry Data corresponding to a player using the LogData structure
	 */
	void AddEntry(const LogData& entry);

	/**
	 * \brief Returns the filename override of the file where the logging results are stored in
	 * \return Name of the file
	 */
	juce::String GetFileNameOverride();

private:

	std::thread thread;
	
	std::atomic<bool> continueLogging{ false };
	std::vector<bool> isUserOperated;

	juce::String logSubfolder = "";
	juce::String logFilenameOverride = "";

	float sampleRate;
	int lineCounter = 0;
	int numPlayers = 0;

	std::vector<LogData> loggingBuffer;
	std::unique_ptr<juce::AbstractFifo> fifo;


	/**
	 * \brief Write header line to log file.
	 *
	 * This includes columns for each player's note onsets, intervals,
	 * user input, delay, and noise parameters. It also includes columns for each pair of players'
	 * asynchronous and alpha/beta parameters. Finally, it includes columns for each player's velocity.
	 *
	 * \param logStream The file stream to write the log to.
	 */
	void WriteHeaders(juce::FileOutputStream& logStream);

	/**
	* \brief Initialises the logging buffer.
	*
	* This is done by creating a AbstractFifo of size 4 * the number of players (or 4 if there are no players),
	* and then resizing the logging buffer to the same size. The buffer is then filled with LogEntry objects,
	* each with asyncs, alphas and betas of size equal to the number of players (or 0 if there are no players),
	* and all elements are set to 0.0.
	*/
	void InitialiseBuffer();

	/**
	 * \brief Starts a logging loop, which writes details of each tap to a file in the user's documents folder.
	 *
	 * This function is run in a separate thread, and continues until the user stops the logger.
	 * TODO: Add checks to ensure that the file is not already open.
	 *
	 * The logging loop is started by calling EnsembleModel::startLoggerLoop().
	 * It can be stopped by calling EnsembleModel::stopLoggerLoop().
	 */
	void loggerLoop();

	/**
	* Logs the onset details of the ensemble to a file.
	*
	* This function is called on a separate thread, so as not to block the audio thread.
	*
	* \param logStream The file stream to write the log to.
	*/
	void logOnsetDetails(juce::FileOutputStream& stream);

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
	void logOnsetDetailsForPlayer(int bufferIndex,
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
		juce::String& velocityLog);

	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Logger)
};
