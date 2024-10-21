#pragma once


#include <JuceHeader.h>
#include <vector>
#include <atomic>
#include <thread>
#include "Player.h"
#include "PluginProcessor.h"


class EnsembleModel;
class AdaptiveMetronomeAudioProcessor;

class XMLManager :
    public juce::ActionBroadcaster
{
private:

    EnsembleModel* ensembleModel;
    AdaptiveMetronomeAudioProcessor* processor;
    juce::MidiFile midiFile;

public:
    XMLManager(EnsembleModel* model, AdaptiveMetronomeAudioProcessor* processor);
    ~XMLManager();

    void XMLManager::loadConfig(juce::File configFile);
    void XMLManager::saveConfig();
    std::unique_ptr<juce::XmlElement> XMLManager::parseXmlConfigFileToXmlElement(juce::File configFile);

};
