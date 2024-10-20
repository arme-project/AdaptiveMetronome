/*
  ==============================================================================

    OSCManager.h
    Created: 17 Oct 2024 4:40:15pm
    Author:  jhund

  ==============================================================================
*/

#pragma once


#include <JuceHeader.h>
#include <vector>
#include <atomic>
#include <thread>
#include "Player.h"
#include "PluginProcessor.h"

// Forward declaration to avoid circular dependencies
class EnsembleModel;

class OSCManager :
    private juce::OSCReceiver,
    private juce::OSCReceiver::ListenerWithOSCAddress<juce::OSCReceiver::MessageLoopCallback>,
    public juce::ActionBroadcaster
{
private:
    
    EnsembleModel* ensembleModel;  // Use the forward-declared class
    juce::OSCSender oscSender;     // Corrected name to avoid confusion with class name

public:
    int currentReceivePort = -1;

    // Constructor and Destructor
    OSCManager(EnsembleModel* model);
    ~OSCManager();

    //==============================================================================    
    // Communication Functions
    void addOSCAddresses();
    void connectOSCSender(int portNumber, juce::String IPAddress);
    void connectOSCReceiver(int portNumber);
    void oscMessageReceived(const juce::OSCMessage& message);
    bool isOscReceiverConnected();
    int getCurrentReceivePort();
    
};