#pragma once

#include <JuceHeader.h>
#include <thread>
#include <atomic>
#include "Player.h"


class Logger 
{
public:

    /**
     * @brief A structure to hold the data for each log entry.
     */
    struct LogData
    {
        int onsetTime = 0;
        int onsetInterval = 0;
        bool userInput = false;
        double delay = 0.0;
        double motorNoise = 0.0;
        double timeKeeperNoise = 0.0;
        std::vector<int> asyncs;
        std::vector<float> alphas;
        std::vector<float> betas;
        double tkNoiseStd = 0.0;
        double mNoiseStd = 0.0;
        double volume = 0.0;
    };

    // Constructor and Deconstructor
    Logger(int numPlayersIn, float sampleRate, const std::vector<bool>& isUserFlags);
    ~Logger();
    
    void Start();
    void Stop();

    void SetFilenameOverride(juce::String filename);
    void SetSubFolder(juce::String filename);
    void AddEntry(const LogData& entry);
    juce::String GetFileNameOverride();
  
private:

    void WriteHeaders(juce::FileOutputStream& logStream);
    void InitialiseBuffer();
    void loggerLoop();
    void logOnsetDetails(juce::FileOutputStream& stream);

    void logOnsetDetailsForPlayer(int bufferIndex,
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

    int numPlayers = 0;
    std::vector<bool> isUserOperated;
    float sampleRate;

    std::atomic<bool> continueLogging{ false };

    std::thread thread;

    juce::String logSubfolder = "";
    juce::String logFilenameOverride = "";

    int lineCounter = 0;
    std::vector<LogData> loggingBuffer;
    std::unique_ptr<juce::AbstractFifo> fifo;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Logger)
};
