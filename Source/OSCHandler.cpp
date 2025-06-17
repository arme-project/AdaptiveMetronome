#include <JuceHeader.h>
#include "OSCHandler.h"

/**
 * \brief Constructor for OSCHandler.
 *
 * \param ensembleModel The Ensemble Model that owns this insstance.
 */
OSCHandler::OSCHandler(EnsembleModel* ensembleModel) :
	model(ensembleModel)
{
	InitialiseAddresses();
}

/**
 * \brief Destructor for the OSCHandler class.
 * Disconnects both sender and receiver objects.
 */
OSCHandler::~OSCHandler()
{
	sender.disconnect();
	receiver.disconnect();
}

/**
* Connect the OSC sender to a UDP port for sending messages to the Max/MSP system.
*
* @param portNumber the UDP port number to connect to
* @param IPAddress the IP address to connect to (default is "127.0.0.1")
*/
void OSCHandler::ConnectSender(int portNumber = 8000, juce::String IPaddress = "127.0.0.1")
{
	if (sender.connect(IPaddress, portNumber)) {
		currentSenderPort = portNumber;
		currentSenderIPAddress = IPaddress;
		DBG("OSC Sender has been connected to " << IPaddress << " on port " << juce::String(portNumber));
	}
	else {
		currentSenderPort = -1;
		currentSenderIPAddress = "";
		DBG("Unable to connect OSC Sender  to " << IPaddress << " on port " << juce::String(portNumber));
	}
}

/**
 * \brief Attempts to connect an OSC receiver to the specified UDP port.
 *
 * \param portNumber The port number to attempt the connection on.
 *
 * If the connection fails, sets currentReceivePort to -1 and logs an error message.
 * If the connection succeeds, sets currentReceivePort to the port number and logs a success message.
 */
void OSCHandler::ConnectReceiver(int portNumber = 8000)
{
	if (receiver.connect(portNumber)) {
		currentReceiverPort = portNumber;
		DBG("OSC Reciever has been connected to listen on port " << juce::String(portNumber));
	}
	else {
		currentReceiverPort = -1;
		DBG("Unable to connect OSC Reciever on port " << juce::String(portNumber));
	}
}

/**
 * \brief Checks if the OSC receiver is connected.
 *
 * \return true if the OSC receiver is connected, false otherwise.
 */
bool OSCHandler::IsSenderConnected() const
{
	return currentSenderPort > -1 && !currentSenderIPAddress.isEmpty();
}

/**
 * \brief Checks if the OSC Sender is connected.
 *
 * \return true if the OSC Sender is connected, false otherwise.
 */
bool OSCHandler::IsReceiverConnected() const
{
	return currentReceiverPort > -1;
}

/**
 * \brief Sends an OSC message either as a test or with onset data.
 *
 * \param test If true, sends a test OSC message to the "/test" address.
 *             Otherwise, sends an OSC message to the "/onsets" address
 *             with four random float arguments.
 */
void OSCHandler::MessageSendTest(juce::String addressPattern)
{
	if (addressPattern == "test") {
		juce::OSCMessage message = juce::OSCMessage("/test");
		if (!sender.send(message))
			DBG("Error: could not send OSC message.");
	}
	else if (addressPattern == "onsets") {
		juce::OSCMessage message = juce::OSCMessage("/onsets");
		for (int i = 0; i < 4; i++) {
			auto randomFloat = 5.0f; // randomizer.nextFloat() / (float)20.0 + (float)0.5;
			message.addArgument(randomFloat);
		}

		if (!sender.send(message)) {
			DBG("Error: could not send OSC message.");
		}
	}
}

/**
 * \brief Responsible for when an OSC message is received by the Reciever and performs corresponding actions base on the pattern
 *
 * \param message The OSC message that has been received.
 */
void OSCHandler::MessageReceived(const juce::OSCMessage& message)
{
	juce::OSCAddressPattern oscPattern = message.getAddressPattern();
	juce::String pattern = oscPattern.toString();

	if (pattern == "/loadConfig") {
		if (message[0].isString()) {
			auto configFilename = message[0].getString();
			auto configSubfolder = model->GetConfigHandler()->GetConfigSubfolder();
			auto configFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(configSubfolder).getChildFile(configFilename);

			if (!configFile.existsAsFile()) { return; }

			model->GetConfigHandler()->LoadConfig(configFile);
		}
	}
	else if (pattern == "/reset") {
		model->reset();
	}
	else if (pattern == "/setLogname") {
		if (message[0].isString())
		{
			juce::String newFileName = message[0].getString();
			if (!newFileName.endsWith(".csv")) {
				newFileName << ".csv";
			}
			model->GetLogger()->SetFilenameOverride(newFileName);
		}
	}
	else if (pattern == "/numIntroTones") {
		if (message[0].isInt32())
		{
			model->SetNumIntroTones(message[0].getInt32());
		}
	}
	else if (pattern == "/plugin") {
		if (message[0].isFloat32() && message[1].isInt32()
			&& message[2].isInt32() && message[3].isFloat32()) {
			float oscOnsetTime = message[0].getFloat32();
			int onsetNoteNumber = message[1].getInt32();
			int msMax = message[2].getInt32();

			if (model->IsManuallyPlaying()) {
				if (model->waitingForFirstNote && onsetNoteNumber == 0) {
					model->triggerFirstNote();
					model->setUserOnsetFromOsc(oscOnsetTime, onsetNoteNumber, msMax);
				}
				else if (onsetNoteNumber > 0) {
					model->setUserOnsetFromOsc(oscOnsetTime, onsetNoteNumber, msMax);
				}
			}
		}
	}
	else if (pattern == "/oscstart") {
		model->reset(true);
		model->SetManualPlaying(true);
	}
	else if (pattern == "/playbackstart") { // Only used to set timer at start of playback. No longer needed.
		if (message[0].isInt32()) {                             // [5]
			if (model->waitingForFirstNote && model->IsManuallyPlaying()) {
				//clock.setStartOfPlayback(message[0].getInt32());
				//DBG("Start playback - " << clock.tickToString(clock.tick()));
			}
		}
	}

	sendActionMessage("OSC Received");
}

/**
 * \brief Initialises the OSC addresses that this handler will listen to.
 *
 * This function sets up the OSC addresses that the handler will listen to for incoming messages.
 * It adds listeners for various addresses related to configuration loading, resetting, and playback control.
 */
void OSCHandler::InitialiseAddresses()
{
	// OSC Listener addresses
	addListener(this, "/loadConfig");
	addListener(this, "/reset");
	addListener(this, "/setLogname");
	addListener(this, "/numIntroTones");

	// OSC Listener addresses for standalone full-system
	addListener(this, "/plugin");     // [4]
	addListener(this, "/oscstart");     // [4]
	addListener(this, "/playbackstart");
	addListener(this, "/alphas");
}

/**
 * Sends an OSC message to notify Max of a new interval. The message contains three integers: the player number, the note number, and the note time in milliseconds.
 * \param playerNum The player number.
 * \param noteNum The note number.
 * \param noteTimeInMS The note time in milliseconds.
 */
void OSCHandler::MessageSendNewInterval(int playerNum, int noteNum, int noteTimeInMS)
{
	auto oscMessage = juce::OSCMessage("/newInterval");
	oscMessage.addInt32(playerNum);
	oscMessage.addInt32(noteNum);
	oscMessage.addInt32(noteTimeInMS);
	if (!sender.send(oscMessage)) {
		DBG("Error: could not send OSC message.");
	}
}

/**
 * \brief Sends an OSC message to trigger a reset in the Max/MSP patch.
 */
void OSCHandler::MessageSendReset() {
	auto oscMessage = juce::OSCMessage("/reset");
	if (!sender.send(oscMessage)) {
		DBG("Error: could not send OSC message.");
	}
}

/**
 * \brief Send an OSC message to trigger Max to start playing.
 *
 * The message is /playMax. This is used to trigger Max to start playing
 * after the user has pressed play in the plugin editor.
 */
void OSCHandler::MessageSendPlayMax() {
	auto oscMessage = juce::OSCMessage("/playMax");
	if (!sender.send(oscMessage)) {
		DBG("Error: could not send OSC message.");
	}
}

/**
 * \brief Sends an action message to the OSC handler.
 *
 * This function broadcasts an action message to all listeners.
 * It is used to notify other components of specific actions or events.
 *
 * \param message The action message to be sent.
 */
void OSCHandler::SendActionMessage(juce::String message)
{
	sendActionMessage(message);
}