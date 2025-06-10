#pragma once

#include <JuceHeader.h>
#include "EnsembleModel.h"

//==============================================================================
/*
*/
class OSCHandler : 
    private juce::OSCReceiver,
    private juce::OSCReceiver::ListenerWithOSCAddress<juce::OSCReceiver::MessageLoopCallback>,
    public juce::ActionBroadcaster
{
public:
    OSCHandler(EnsembleModel* ensembleModel);
    ~OSCHandler() override;

    void ConnectSender(int portNumber = 8000, juce::String IPaddress = "127.0.0.1");
    void ConnectReceiver(int portNumber = 8001);

    bool IsReceiverConnected() const;
    bool IsSenderConnected() const;

    // Testing Function
    void MessageSendTest(juce::String pattern = "test");

private:
    
    void MessageReceived(const juce::OSCMessage &message);
    void InitialiseAddresses();

    void MessageSendNewInterval(int playerNum, int noteNum, int noteTimeInMS);
    void MessageSendReset();
    void MessageSendPlayMax();

    juce::OSCSender sender;
    juce::OSCReceiver receiver;

    int currentSenderPort = -1;
    juce::String currentSenderIPAddress = "";
    int currentReceiverPort = -1;


    EnsembleModel* model;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSCHandler)
};
