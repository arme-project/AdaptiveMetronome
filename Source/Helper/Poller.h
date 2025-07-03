#pragma once

#include <JuceHeader.h>

/**
 * \class Poller
 * \brief Handles Polling of ensemble model data.
 *
 * This class is responsible for polling the ensemble model data at regular intervals.
 * It runs in a separate thread to avoid blocking the audio thread, and uses an AbstractFifo
 * to manage the polling buffer.
 */
class Poller
{
public:
	/**
	 * \brief Constructor for the Poller Class.
	 *
	 * \input numPlayersIn Number of Players in the Ensemble
	 */
	Poller(int numPlayersIn);

	/**
	 * \brief Deconstructor for the Poller Class.
	 */
	~Poller();

	/**
	 * \brief Starts the polling loop for the ensemble model.
	 *
	 * This function stops any existing polling loop, initialises the polling buffers,
	 * and starts a new thread to run the polling loop. The polling loop will continue
	 * to run until the stopPollingLoop function is called.
	 */
	void Start();

	/**
	 * \brief Stops the polling loop by setting the continuePolling flag to false.
	 *
	 * If the polling thread is joinable, it will join the thread to ensure proper cleanup.
	 */
	void Stop();

private:
	int numPlayers;
	std::unique_ptr<juce::AbstractFifo> fifo;
	std::vector<std::vector<float>> buffer;
	std::thread thread;
	std::atomic<bool> continuePolling;
	std::atomic_flag alphasUpToDate;

	/**
	 * \brief Initialise the polling buffers for the players.
	 *
	 * Allocates memory for the polling buffers and initialises them to zero. The size of each buffer is 10 times
	 * the number of players in the ensemble.
	 */
	void InitialiseBuffers();

	/**
	 * \brief The polling loop that runs in a separate thread which continuously checks if the alphas are up to date.
	 *
	 * This function continuously checks if the alphas are up to date. If they are not,
	 * it calls getNewAlphas() to fetch the latest alpha values. The loop continues until
	 * continuePolling is set to false.
	 */
	void PollingLoop();

	/**
	 * \brief Fetches new alpha values from the server (NOT USED).
	 *
	 * This function should make a request to the server to get new alpha values for the players.
	 * If new values are received, they are stored in the buffer. If no new values are received,
	 * the alphasUpToDate flag is cleared to allow for another polling attempt later.
	 */
	void getNewAlphas();

	/**
	 * \brief NOT IMPLEMENTED.
	 *
	 */
	void getLatestAlphas();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Poller)
};
