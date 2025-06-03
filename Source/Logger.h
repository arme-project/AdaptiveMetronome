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
        int onsetTime = 0, onsetInterval = 0;
        bool userInput = false;
        double delay = 0.0;
        double motorNoise = 0.0, timeKeeperNoise = 0.0;
        std::vector<int> asyncs;
        std::vector<float> alphas, betas;
        double tkNoiseStd = 0.0, mNoiseStd = 0.0;
        double volume = 0.0;
    };

    // Constructor and Deconstructor
    Logger(int numPlayers);
    ~Logger() override;

    void start();
    void stop();


private:
    std::thread loggerThread;
    std::atomic<bool> continueLogging{ false };
    int lineCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Logger)
};
