#pragma once
#include <JuceHeader.h>
#include <vector>
#include <atomic>
#include <thread>
#include "Player.h"

class EnsembleModel
{
public:
    //==============================================================================
    EnsembleModel();
    ~EnsembleModel();
    
    //==============================================================================
    bool loadMidiFile (const juce::File &file);
    bool reset();
    
    //==============================================================================
    void prepareToPlay (double newSampleRate);
    void releaseResources();
    
    //==============================================================================
    void processMidiBlock (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int numSamples, double tempo);
    
    //==============================================================================
    int getNumPlayers();
    bool isPlayerUserOperated (int playerIndex);
    juce::AudioParameterInt& getPlayerChannelParameter (int playerIndex);
    juce::AudioParameterFloat& getPlayerDelayParameter (int playerIndex);
    juce::AudioParameterFloat& getPlayerMotorNoiseParameter (int playerIndex);
    juce::AudioParameterFloat& getPlayerTimeKeeperNoiseParameter (int playerIndex);
    juce::AudioParameterFloat& getPlayerVolumeParameter (int playerIndex);
    juce::AudioParameterFloat& getAlphaParameter (int player1Index, int player2Index);
    
    //==============================================================================
    static void soundOffAllChannels (juce::MidiBuffer &midi);

private:
    //==============================================================================
    static const int numUserPlayers = 1;
    
    //==============================================================================
    // Timing parameters
    double sampleRate = 44100.0;
    int samplesPerBeat = sampleRate / 4;
    int scoreCounter = 0;
    std::vector <std::vector <std::unique_ptr <juce::AudioParameterFloat> > > alphaParams;
    
    //==============================================================================
    // Intro countdown
    static const int numIntroTones = 4;
    static const int introToneNote = 69;
    static const juce::uint8 introToneVel = 100;
    int introCounter = 0;
    int introTonesPlayed = 0;
    
    void playIntroTones (juce::MidiBuffer &midi, int sampleIndex);
    void introToneOn (juce::MidiBuffer &midi, int sampleIndex);
    void introToneOff (juce::MidiBuffer &midi, int sampleIndex);
    
    //==============================================================================
    // Funtions for ammendinding timings for each player in this ensemble. These
    // should only be called from within processMidiBlock().
    bool initialTempoSet = false;
    void setTempo (double bpm);
    void setInitialPlayerTempo();
    
    bool newOnsetsAvailable();
    void calculateNewIntervals();
    void clearOnsetsAvailable();
        
    void getLatestAlphas();
    void storeOnsetDetailsForPlayer (int bufferIndex, int playerIndex);
    
    //==============================================================================
    // Ensemble players and associated parameters.
    class FlagLock
    {
    public:
        FlagLock (std::atomic_flag &f);
        ~FlagLock();
        
        std::atomic_flag &flag;
        bool locked;
    };
    
    // The following functions should only be called when the playersInUse
    // flag has been locked using the above FlagLock class.
    std::vector <std::unique_ptr <Player> > players;
    std::atomic_flag playersInUse;

    void createPlayers (const juce::MidiFile &file);
    void createAlphaParameters();
    
    void playScore (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex);
    
    std::atomic_flag resetFlag;
    void resetPlayers();
    
    //==============================================================================
    // A bunch of stuff for safely logging onset times and sending them out to the
    // server. Functions defined in here are only safe to call from the logging thread.
    std::unique_ptr <juce::AbstractFifo> loggingFifo;
    
    struct LogData
    {
        int onsetTime, onsetInterval;
        bool userInput;
        double motorNoise, timeKeeperNoise;
        std::vector <int> asyncs;
        std::vector <float> alphas;
        double tkNoiseStd, mNoiseStd;
        double volume;
    };
    
    std::vector <LogData> loggingBuffer;

    void initialiseLoggingBuffer();
    
    std::thread loggerThread;
    std::atomic <bool> continueLogging;
        
    void startLoggerLoop();
    void stopLoggerLoop();
    
    int logLineCounter = 0;
    
    void loggerLoop();
    void writeLogHeader (juce::FileOutputStream &logStream);
    void logOnsetDetails (juce::FileOutputStream &logStream);
    
    void logOnsetDetailsForPlayer (int bufferIndex,
                                   juce::String &onsetLog,
                                   juce::String &intervalLog,
                                   juce::String &userInputLog,
                                   juce::String &mNoiseLog,
                                   juce::String &tkNoiseLog,
                                   juce::String &asyncLog,
                                   juce::String &alphaLog,
                                   juce::String &tkNoiseStdLog,
                                   juce::String &mNoiseStdLog,
                                   juce::String &velocityLog);
                                   
    void postLatestOnsets (const std::vector <int> &onsets);
                                   
    //==============================================================================
    // Functionality for polling for new alpha values from the server.
    std::unique_ptr <juce::AbstractFifo> pollingFifo;
    std::vector <std::vector <float> > pollingBuffer;

    void initialisePollingBuffers();
    
    std::thread pollingThread;
    std::atomic <bool> continuePolling;
    std::atomic_flag alphasUpToDate;
    
    void startPollingLoop();
    void stopPollingLoop();
    
    void pollingLoop();
    void getNewAlphas();
    
    //==============================================================================
    static bool checkMidiSequenceHasNotes (const juce::MidiMessageSequence *seq);
};
