#pragma once
#include <JuceHeader.h>
#include <vector>
#include <atomic>
#include <thread>
#include "Player.h"
#include "MetronomeClock.h"
#include "MatlabEngine.hpp"
#include "MatlabDataArray.hpp"
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>
#include "recalculateAlphas.h"

using namespace matlab::engine;

class EnsembleModel
{
public:
    //==============================================================================
    EnsembleModel();
    ~EnsembleModel();
    
    //==============================================================================
    // Communication
    std::unique_ptr<MATLABEngine> matlabEngine;
    juce::OSCSender sender;
    void writeToLogger(time_point<system_clock> timeStamp, juce::String source, juce::String method, juce::String message);
    void oscMessageSend(bool test);

    void oscMessageSendNewInterval(int playerNum, int noteNum, int noteTimeInMS);
    void oscMessageSendReset();
    void oscMessageSendPlayMax();

    // MATLAB integration
    matlab::data::ArrayFactory factory;
    bool getAlphasFromMATLAB(bool test);
    bool getAlphasFromCodegen(bool test);

    bool setAlphasFromMATLABArray(matlab::data::TypedArray<double> alphasFromMATLAB);
    bool setAlphasFromCodegen(std::vector<std::vector<double>> alphasFromCodegen);
    std::vector<matlab::data::Array> buildMatlabOnsetArray(bool test);

    //==============================================================================
    // Metronome for accurate time measurements
    juce::Random randomizer;
    MetronomeClock* clock = nullptr;
    void setMetronomeClock(MetronomeClock* clockPtr);
    bool logToLogger = false;

    //==============================================================================
    bool loadMidiFile (const juce::File &file, int userPlayers);
    bool reset();
    bool reset(bool skipIntroNotes);
    void setTempo (double bpm);
    
    //==============================================================================
    void prepareToPlay (double newSampleRate);
    void releaseResources();
    
    //==============================================================================
    void processMidiBlock (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int numSamples, double tempo);
    void setUserOnsetFromOsc(float oscOnsetTime, int onsetNoteNumber, int msMax);
    //==============================================================================
    bool waitingForFirstNote = true;
    void triggerFirstNote();
    int getNumPlayers();
    bool isPlayerUserOperated (int playerIndex);
    juce::AudioParameterInt& getPlayerChannelParameter (int playerIndex);
    juce::AudioParameterFloat& getPlayerDelayParameter (int playerIndex);
    juce::AudioParameterFloat& getPlayerMotorNoiseParameter (int playerIndex);
    juce::AudioParameterFloat& getPlayerTimeKeeperNoiseParameter (int playerIndex);
    juce::AudioParameterFloat& getPlayerVolumeParameter (int playerIndex);
    juce::AudioParameterFloat& getAlphaParameter (int player1Index, int player2Index);
    
    // TODO: Change to a JUCE parameter
    juce::Atomic<int> currentNoteIndex;
    
    //==============================================================================
    static void soundOffAllChannels (juce::MidiBuffer &midi);
    std::vector <std::unique_ptr <Player> > players;


private:
    //==============================================================================
    int numUserPlayers = 1;
    
    //==============================================================================
    // Timing parameters
    double sampleRate = 44100.0;
    int samplesPerBeat = sampleRate / 4;
    int scoreCounter = 0;
    std::vector <std::vector <std::unique_ptr <juce::AudioParameterFloat> > > alphaParams;
    std::vector <std::vector <std::unique_ptr <juce::AudioParameterFloat> > > alphaParamsFixed;

    //==============================================================================
    // Intro countdown
    static const int numIntroTones = 4;
    static const int introToneNote = 69;
    static const juce::uint8 introToneVel = 100;
    int introCounter = 0;

    int introTonesPlayed = 0;
    bool playbackStarted = false;
    bool introFinishedPlaying = false;

    bool firstSampleProcessed = false;

    void playIntroTones (juce::MidiBuffer &midi, int sampleIndex);
    void introToneOn (juce::MidiBuffer &midi, int sampleIndex);
    void introToneOff (juce::MidiBuffer &midi, int sampleIndex);
    
    //==============================================================================
    // Funtions for ammendinding timings for each player in this ensemble. These
    // should only be called from within processMidiBlock().
    bool initialTempoSet = false;
    void setInitialPlayerTempo();
    
    bool newOnsetsAvailable();
    void calculateNewIntervals();
    void clearOnsetsAvailable();
        

    //void getLatestAlphas();
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
        int onsetTime, onsetIntervalForNextNote;
        bool userInput;
        double delay;
        double motorNoise, timeKeeperNoise;
        std::vector <int> asyncs;
        std::vector <float> alphas;
        double tkNoiseStd, mNoiseStd;
        double volume;
    };
    
    std::vector <LogData> loggingBuffer;

    void initialiseLoggingBuffer();
    
    std::thread loggerThread;
    std::thread pollingThread;

    std::atomic <bool> continueLogging;
    std::atomic <bool> continuePolling;

    void startLoggerLoop();
    void stopLoggerLoop();

    void startPollingLoop();
    void stopPollingLoop();

    int logLineCounter = 0;
    
    void loggerLoop();
    void pollingLoop();
    void writeLogHeader (juce::FileOutputStream &logStream);
    void logOnsetDetails (juce::FileOutputStream &logStream);
    
    void logOnsetDetailsForPlayer (int bufferIndex,
                                   juce::String &onsetLog,
                                   juce::String &intervalLog,
                                   juce::String &userInputLog,
                                   juce::String &delayLog,
                                   juce::String &mNoiseLog,
                                   juce::String &tkNoiseLog,
                                   juce::String &asyncLog,
                                   juce::String &alphaLog,
                                   juce::String &tkNoiseStdLog,
                                   juce::String &mNoiseStdLog,
                                   juce::String &velocityLog);
                                   
    void postLatestOnsets (const std::vector <int> &onsets, const std::vector <int> &delays);
                                   
    //==============================================================================
    // Functionality for polling for new alpha values from the server.

    std::atomic_flag alphasUpToDate;
    
    //==============================================================================
    static bool checkMidiSequenceHasNotes (const juce::MidiMessageSequence *seq);
};
