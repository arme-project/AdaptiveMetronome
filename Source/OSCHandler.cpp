#include <JuceHeader.h>
#include "OSCHandler.h"

OSCHandler::OSCHandler(EnsembleModel* ensembleModel) :
	model(ensembleModel)
{
	InitialiseAddresses();
}

OSCHandler::~OSCHandler()
{
	sender.disconnect();
	reciever.disconnect();
}

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

void OSCHandler::ConnectReceiver(int portNumber = 8000)
{
	if (reciever.connect(portNumber)) {
		currentReceiverPort = portNumber;
		DBG("OSC Reciever has been connected to listen on port " << juce::String(portNumber));
	}
	else {
		currentReceiverPort = -1;
		DBG("Unable to connect OSC Reciever on port " << juce::String(portNumber));
	}
}

bool OSCHandler::IsReceiverListening() const
{
	return currentReceiverPort > - 1;
}

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

void OSCHandler::MessageReceived(const juce::OSCMessage &message)
{
	juce::OSCAddressPattern oscPattern = message.getAddressPattern();
	juce::String pattern = oscPattern.toString();

	if (pattern == "/loadConfig") {}
	else if (pattern == "/reset") {}
	else if (pattern == "/setLogname") {}
	else if (pattern == "/numIntroTones") {}
	else if (pattern == "/plugin") {}
	else if (pattern == "/oscstart") {}
	else if (pattern == "/playbackstart") {}
	else if (pattern == "/alphas") {}
}

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
