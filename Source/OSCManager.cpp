/*
  ==============================================================================

    OSCManager.cpp
    Created: 17 Oct 2024 4:40:15pm
    Author:  jhund

  ==============================================================================
*/

#include "OSCManager.h"

//==============================================================================

OSCManager::OSCManager(EnsembleModel* model)
{
	ensembleModel = model; // Used to call for functions that are within the calling object
}

OSCManager::~OSCManager() = default;

//==============================================================================

void OSCManager::addOSCAddresses() {
	addListener(this, "/loadConfig");
	addListener(this, "/reset");
	addListener(this, "/setLogname");
	addListener(this, "/numIntroTones");
}

void OSCManager::connectOSCSender(int portNumber, juce::String ip) {
	if (oscSender.connect(ip, portNumber)) {  // Fixed naming here
		DBG("OSC SENDER CONNECTED");
	}
	else {
		DBG("Error: could not connect to UDP port 8000.");
	}
}

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

bool OSCManager::isOscReceiverConnected() {
	return (currentReceivePort > -1);
}

