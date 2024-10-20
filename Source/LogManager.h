#pragma once
#include <JuceHeader.h>
#include <vector>
#include <atomic>
#include <thread>
#include "Player.h"
#include "PluginProcessor.h"


class EnsembleModel;
class AdaptiveMetronomeAudioProcessor;

class LogManager :
    public juce::ActionBroadcaster
{
private:

    EnsembleModel* ensembleModel;
    AdaptiveMetronomeAudioProcessor* processor;

public:
    LogManager(EnsembleModel* model);
    ~LogManager();

    void LogManager::initialiseLoggingBuffer();
    void LogManager::startLoggerLoop();
    void LogManager::stopLoggerLoop();
    void LogManager::loggerLoop();
    void LogManager::writeLogHeader(juce::FileOutputStream& logStream);
    void LogManager::logOnsetDetails(juce::FileOutputStream& logStream);
    void LogManager::logOnsetDetailsForPlayer(int bufferIndex,
        juce::String& onsetLog,
        juce::String& intervalLog,
        juce::String& userInputLog,
        juce::String& delayLog,
        juce::String& mNoiseLog,
        juce::String& tkNoiseLog,
        juce::String& asyncLog,
        juce::String& alphaLog,
        juce::String& betaLog,
        juce::String& tkNoiseStdLog,
        juce::String& mNoiseStdLog,
        juce::String& velocityLog);
    void LogManager::postLatestOnsets(const std::vector <int>& onsets, const std::vector <int>& delays);


};
