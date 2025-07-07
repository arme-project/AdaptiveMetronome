#pragma once
#include <JuceHeader.h>

// Forward declare EnsembleModel to avoid circular includes.
class EnsembleModel;

/**
 * \class OSCHandler
 * \brief Handles OSC communication for the Ensemble Model.
 *
 * This class is responsible for sending and receiving OSC messages to and from a Max/MSP system.
 * It manages the connection to the OSC sender and receiver, and provides methods to send specific messages.
 */
class OSCHandler :
	private juce::OSCReceiver,
	private juce::OSCReceiver::ListenerWithOSCAddress<juce::OSCReceiver::MessageLoopCallback>,
	public juce::ActionBroadcaster
{
public:
	/**
	 * \brief Constructor for OSCHandler.
	 *
	 * \param ensembleModel The Ensemble Model that owns this insstance.
	 */
	OSCHandler(EnsembleModel* ensembleModel);

	/**
	 * \brief Destructor for the OSCHandler class.
	 * Disconnects both sender and receiver objects.
	 */
	~OSCHandler() override;

	/**
	* \brief Connect the OSC sender to a UDP port for sending messages to the Max/MSP system.
	*
	* \param portNumber the UDP port number to connect to
	* \param IPAddress the IP address to connect to (default is "127.0.0.1")
	*/
	void ConnectSender(int portNumber = 8000, juce::String IPaddress = "127.0.0.1");
	
	/**
	* \brief Attempts to connect an OSC receiver to the specified UDP port.
	*
	* \param portNumber The port number to attempt the connection on.
	*
	* If the connection fails, sets currentReceivePort to -1 and logs an error message.
	* If the connection succeeds, sets currentReceivePort to the port number and logs a success message.
	*/
	void ConnectReceiver(int portNumber = 8070);

	/**
	 * \brief Checks if the OSC receiver is connected.
	 *
	 * \return true if the OSC receiver is connected, false otherwise.
	 */
	bool IsReceiverConnected() const;

	/**
	 * \brief Checks if the OSC Sender is connected.
	 *
	 * \return true if the OSC Sender is connected, false otherwise.
	 */
	bool IsSenderConnected() const;

	/**
	 * \brief Sends an OSC message to notify Max of a new interval. The message contains three integers: the player number, the note number, and the note time in milliseconds.
	 *
	 * \param playerNum The player number.
	 * \param noteNum The note number.
	 * \param noteTimeInMS The note time in milliseconds.
	 */
	void MessageSendNewInterval(int playerNum, int noteNum, int noteTimeInMS);

	/**
	 * \brief Sends an OSC message to trigger a reset in the Max/MSP patch.
	 */
	void MessageSendReset();

	/**
	 * \brief Send an OSC message to trigger Max to start playing.
	 *
	 * The message is /playMax. This is used to trigger Max to start playing
	 * after the user has pressed play in the plugin editor.
	 */
	void MessageSendPlayMax();

	/**
	 * \brief Sends an action message from the OSC handler.
	 *
	 * This function broadcasts an action message to all listeners.
	 * It is used to notify other components of specific actions or events.
	 *
	 * \param message The action message to be sent.
	 */
	void SendActionMessage(juce::String message);

	/**
	 * \brief Returns current receiver port.
	 *
	 * This function returns the current receiver port number that the OSCHandler is listening on.
	 */
	int GetCurrentReceiverPort();

	/**
	 * \brief Returns current receiver port.
	 *
	 * This function returns the current receiver port number that the OSCHandler is listening on.
	 */
	int GetCurrentSenderPort();

	// Testing Function
	/**
	 * \brief Sends an OSC message either as a test or with onset data.
	 *
	 * \param test If true, sends a test OSC message to the "/test" address.
	 *             Otherwise, sends an OSC message to the "/onsets" address
	 *             with four random float arguments.
	 */
	void MessageSendTest(juce::String pattern = "test");



private:

	EnsembleModel* model;

	juce::OSCSender sender;
	juce::OSCReceiver receiver;
	
	int currentSenderPort = -1;
	juce::String currentSenderIPAddress = "";
	int currentReceiverPort = -1;

	/**
	 * \brief Responsible for when an OSC message is received by the Reciever and performs corresponding actions base on the pattern
	 *
	 * \param message The OSC message that has been received.
	 */
	//void MessageReceived(const juce::OSCMessage& message);
	void oscMessageReceived(const juce::OSCMessage& message) override;

	/**
	 * \brief Initialises the OSC addresses that this handler will listen to.
	 *
	 * This function sets up the OSC addresses that the handler will listen to for incoming messages.
	 * It adds listeners for various addresses related to configuration loading, resetting, and playback control.
	 */
	void InitialiseAddresses();

	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSCHandler)
};
