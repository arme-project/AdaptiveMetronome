#pragma once

#include <JuceHeader.h>

class Poller  : public juce::Component
{
public:
    Poller(int numPlayersIn);
    ~Poller() override;

    void Start();
    void Stop();

private:

    int numPlayers = 0;

    void InitialiseBuffers();
    void PollingLoop();
    void getNewAlphas();
    void getLatestAlphas();

    std::unique_ptr<juce::AbstractFifo> fifo;
    std::vector<std::vector<float>> buffer;
    std::thread thread;
    std::atomic<bool> continuePolling;
    std::atomic_flag alphasUpToDate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Poller)
};
