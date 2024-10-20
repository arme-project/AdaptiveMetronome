#pragma once
#include <JuceHeader.h>
#include <vector>
#include <atomic>
#include <thread>
#include "Player.h"


using std::function;

class AdaptiveMetronomeAudioProcessor;

using AudioParameterFloatToUse = juce::AudioParameterFloat;

class EnsembleModel :
    private juce::OSCReceiver,
    private juce::OSCReceiver::ListenerWithOSCAddress <juce::OSCReceiver::MessageLoopCallback>,
    public juce::ActionBroadcaster
{
public:
    //==============================================================================
    EnsembleModel();
    EnsembleModel(AdaptiveMetronomeAudioProcessor *processorPtr);

    ~EnsembleModel();
    
    AdaptiveMetronomeAudioProcessor *processor = nullptr;
    //==============================================================================
    bool loadMidiFile (const juce::File &file, int userPlayers);
    bool reset();
    
    //==============================================================================
    void prepareToPlay (double newSampleRate);
    void releaseResources();
    
    //==============================================================================
    void processMidiBlock (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int numSamples, double tempo);
    
    //==============================================================================
    int getNumPlayers();
    int getNumUserPlayers();
    bool isPlayerUserOperated (int playerIndex);
    juce::AudioParameterInt& getPlayerChannelParameter (int playerIndex);
    AudioParameterFloatToUse& getPlayerDelayParameter (int playerIndex);
    AudioParameterFloatToUse& getPlayerMotorNoiseParameter (int playerIndex);
    AudioParameterFloatToUse& getPlayerTimeKeeperNoiseParameter (int playerIndex);
    AudioParameterFloatToUse& getPlayerVolumeParameter (int playerIndex);
    AudioParameterFloatToUse& getAlphaParameter (int player1Index, int player2Index);
    AudioParameterFloatToUse& getBetaParameter (int player1Index, int player2Index);

    //==============================================================================
    static void soundOffAllChannels (juce::MidiBuffer &midi);


    // Functions for storing and loading ensemble config from XML file
    void saveConfigToXmlFile();
    std::unique_ptr<juce::XmlElement> parseXmlConfigFileToXmlElement(juce::File configFile);
    // Load from parsed xml
    void loadConfigFromXml(std::unique_ptr<juce::XmlElement> loadedConfig);
    // Load direct from File (.xml)
    void loadConfigFromXml(juce::File configFile);

    // Folder structure
    juce::String logSubfolder = "";
    juce::String configSubfolder = "";
    juce::String logFilenameOverride = "";

    // OSC Messaging
    juce::OSCSender OSCSender;
    void connectOSCSender(int portNumber, juce::String IPAddress);
    void connectOSCReceiver(int portNumber);
    void oscMessageReceived(const juce::OSCMessage& message);
    int currentReceivePort = -1;
    bool isOscReceiverConnected();

    void setAlphaBetaParams(float valueIn);
    
private:
    //==============================================================================
    int numUserPlayers = 1;
    
    // Previously a local variable in loadMidifile()
    juce::MidiFile midiFile;

    //==============================================================================
    // Timing parameters
    double sampleRate = 44100.0;
    int samplesPerBeat = sampleRate / 4;
    int scoreCounter = 0;
    
    //==============================================================================
    // Intro countdown
    const int introToneChannel = 16;
    int numIntroTones = 4;
    static const int introToneNoteFirst = 84;
    static const int introToneNoteOther = 72;
    static const juce::uint8 introToneVel = 100;
    int introCounter = 0;
    int introTonesPlayed = 0;
    
    void playIntroTones (juce::MidiBuffer &midi, int sampleIndex);
    void introToneOn (juce::MidiBuffer &midi, int sampleIndex);
    void introToneOff (juce::MidiBuffer &midi, int sampleIndex);
    void introToneOnOff (juce::MidiBuffer &midi, juce::MidiMessage (*function)(int, int, juce::uint8), int sampleIndex);
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
    void createAlphaBetaParameters();
    
    void playScore (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex);
    void playUserIntro(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int sampleIndex);
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
        double delay;
        double motorNoise, timeKeeperNoise;
        std::vector <int> asyncs;
        std::vector <float> alphas;
        std::vector <float> betas;
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
                                   juce::String &delayLog,
                                   juce::String &mNoiseLog,
                                   juce::String &tkNoiseLog,
                                   juce::String &asyncLog,
                                   juce::String &alphaLog,
                                   juce::String &betaLog,
                                   juce::String &tkNoiseStdLog,
                                   juce::String &mNoiseStdLog,
                                   juce::String &velocityLog);
                                   
    void postLatestOnsets (const std::vector <int> &onsets, const std::vector <int> &delays);
                                   
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
