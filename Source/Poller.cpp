#include <JuceHeader.h>
#include "Poller.h"

//==============================================================================
/**
 * \brief Constructor for the Poller Class.
 *
 * \input numPlayersIn Number of Players in the Ensemble
 */
Poller::Poller(int numPlayersIn) : numPlayers(numPlayers)
{
}

/**
 * \brief Deconstructor for the Poller Class.
 */
Poller::~Poller()
{
	Stop();
}

/**
 * \brief Starts the polling loop for the ensemble model.
 *
 * This function stops any existing polling loop, initialises the polling buffers,
 * and starts a new thread to run the polling loop. The polling loop will continue
 * to run until the stopPollingLoop function is called.
 */
void Poller::Start()
{
	Stop();
	InitialiseBuffers();

	continuePolling = true;
	alphasUpToDate.test_and_set();
	thread = std::thread([this]() {this->PollingLoop(); });
}

/**
* \brief Stops the polling loop by setting the continuePolling flag to false.
*
* If the polling thread is joinable, it will join the thread to ensure proper cleanup.
*/
void Poller::Stop()
{
	continuePolling = false;
	if (thread.joinable())
	{
		thread.join();
	}
}

/**
 * \brief Initialise the polling buffers for the players.
 *
 * Allocates memory for the polling buffers and initialises them to zero. The size of each buffer is 10 times
 * the number of players in the ensemble.
 */
void Poller::InitialiseBuffers()
{
	auto bufferSize = 10 * numPlayers;
	fifo = std::make_unique <juce::AbstractFifo>(bufferSize);
	buffer.resize(numPlayers);

	for (int i = 0; i < numPlayers; ++i)
	{
		buffer[i].resize(bufferSize, 0.0);
	}
}

/**
 * \brief Stops the polling loop by setting the continuePolling flag to false.
 *
 * If the pollingThread is joinable, it will join the thread to ensure
 * proper cleanup and prevent any dangling threads.
 */
void Poller::PollingLoop()
{
	while (continuePolling) {
		if (!alphasUpToDate.test_and_set()) {
			getNewAlphas();
		}
	}
}

/**
 * \brief NOT USED
 */
void Poller::getNewAlphas()
{
	//==========================================================================
	// In here you should make a request to your server to ask for new alpha
	// values. If you get some updated values set the following value to true.
	// If not, set the value to false and the plug-in will poll again after
	// short time.

	if (bool newAlphas = false) {
		auto writer = fifo->write(static_cast <int> (numPlayers));

		for (int player1 = 0; player1 < buffer.size(); ++player1) {
			int player2 = 0;

			for (int i = 0; i < writer.blockSize1; ++i) {
				// Replace the 0.2 with the alpha parameter for player1_player2
				buffer[player1][writer.startIndex1 + i] = 0.2;
				++player2;
			}

			for (int i = 0; i < writer.blockSize2; ++i)
			{
				// Replace the 0.2 with the alpha parameter for player1_player2
				buffer[player1][writer.startIndex2 + i] = 0.2;
				++player2;
			}
		}
	}
	else {
		alphasUpToDate.clear();
	}
}

// NOT IMPLEMENTED
void Poller::getLatestAlphas()
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