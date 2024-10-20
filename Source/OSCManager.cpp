#include "OSCManager.h"

//==============================================================================

OSCManager::OSCManager(EnsembleModel* model)
	: ensembleModel(model)
{
}

OSCManager::~OSCManager() = default;

//==============================================================================

void OSCManager::addOSCAddresses() {
	addListener(this, "/loadConfig");
	addListener(this, "/reset");
	addListener(this, "/setLogname");
	addListener(this, "/numIntroTones");
}

// Establishes a connection to an OSC sender at the specified IP address and port number.
void OSCManager::connectOSCSender(int portNumber, juce::String ip) {
	if (oscSender.connect(ip, portNumber)) {  // Fixed naming here
		DBG("OSC SENDER CONNECTED");
	}
	else {
		DBG("Error: could not connect to UDP port 8000.");
	}
}

// Establishes a connection to an OSC receiver at the specified port number.
// If the connection is successful, the current receive port  is updated, otherwise an error is logged.
void OSCManager::connectOSCReceiver(int portNumber) {
	if (!connect(portNumber)) {
		currentReceivePort = -1;
		DBG("Error: could not connect to UDP.");
	}
	else {
		sendActionMessage("OSC Received");
		currentReceivePort = portNumber;
		DBG("Connection succeeded");
	}
}

// Handles the retrieval of OSC messages.
// The function processes specific OSC addresses and performs actions like loading configuration files, resetting the model,
// setting log filenames, and updating the number of intro tones.
void OSCManager::oscMessageReceived(const juce::OSCMessage& message) {
	juce::OSCAddressPattern oscPattern = message.getAddressPattern();
	juce::String oscAddress = oscPattern.toString();

	if (oscAddress == "/loadConfig") {
		if (message[0].isString()) {
			auto configFilename = message[0].getString();
			auto configFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(ensembleModel->configSubfolder).getChildFile(configFilename); // Assuming configSubfolder is a member of ensembleModel

			if (!configFile.existsAsFile()) { return; }

			ensembleModel->loadConfigFromXml(configFile);
		}
	}
	else if (oscAddress == "/reset") {
		ensembleModel->reset();
	}
	else if (oscAddress == "/setLogname") {
		if (message[0].isString()) {
			auto newLogFilename = message[0].getString();
			if (!newLogFilename.endsWith(".csv")) {
				newLogFilename << ".csv";
			}
			ensembleModel->logFilenameOverride = newLogFilename;
		}
	}
	else if (oscAddress == "/numIntroTones") {
		if (message[0].isInt32()) {
			ensembleModel->numIntroTones = message[0].getInt32();
		}
	}
	sendActionMessage("OSC Received");
}

// Checks if the OSC receiver is currently connected by verifying if the port number is valid.
bool OSCManager::isOscReceiverConnected() {
	return (currentReceivePort > -1);
}

int OSCManager::getCurrentReceivePort()
{
	return currentReceivePort;
}
