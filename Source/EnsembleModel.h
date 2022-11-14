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
    
    //==============================================================================
    void setSampleRate (double newSampleRate);
    
    //==============================================================================
    void processMidiBlock (juce::MidiBuffer &midi, int numSamples, double tempo);

private:
    //==============================================================================
    // Players
    std::vector <std::unique_ptr <Player> > players;
    
    //==============================================================================
    // Timing parameters
    double sampleRate = 44100.0;
    int samplesPerBeat = sampleRate / 4;
    int scoreCounter = 0;
    std::vector <std::vector <double> > alphas;
    
    //==============================================================================
    bool initialTempoSet = false;
    void setTempo (double bpm);
    void setInitialPlayerTempo();
    
    bool newOnsetsAvailable();
    void calculateNewIntervals();
    void clearOnsetsAvailable();
    
    //==============================================================================
    class FlagLock
    {
    public:
        FlagLock (std::atomic_flag &f);
        ~FlagLock();
        
        std::atomic_flag &flag;
        bool locked;
    };
    
    std::atomic_flag playersInUse;
    void clearPlayers();
    void createPlayers (const juce::MidiFile &file);
    void initialiseAlphas();
    
    //==============================================================================
    // A bunch of stuff for safely logging onset times and sending them out to the
    // server.
    std::unique_ptr <juce::AbstractFifo> onsetFifo;
    std::vector <int> onsetBuffer;
    
    void initialiseOnsetBuffer (int bufferSize);
    
    std::thread loggerThread;
    std::atomic <bool> continueLogging;
        
    void startLoggerLoop();
    void stopLoggerLoop();
    
    void loggerLoop();
    void logOnsets (juce::FileOutputStream &logStream);

    //==============================================================================
    static bool checkMidiSequenceHasNotes (const juce::MidiMessageSequence *seq);
};
