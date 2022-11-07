#pragma once
#include <JuceHeader.h>
#include <vector>
#include "Player.h"

class EnsembleModel
{
public:
    //==============================================================================
    EnsembleModel();
    ~EnsembleModel();
    
    //==============================================================================
    void loadMidiFile (const juce::File &file);
    
    //==============================================================================
    void setSampleRate (double newSampleRate);
    void setTempo (double bpm);
    
    //==============================================================================
    void processMidiBlock (juce::MidiBuffer &midi, int numSamples);

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
    bool newOnsetsAvailable();
    void calculateNewIntervals();
    void clearOnsetsAvailable();
    
    //==============================================================================
    void clearPlayers();
    void createPlayers (const juce::MidiFile &file);
    void initialiseAlphas();

    //==============================================================================
    static bool checkMidiSequenceHasNotes (const juce::MidiMessageSequence *seq);
    
    
    
    
    //==============================================================================
    void debugLog (double test)
    {
        juce::File log ("/Users/Sean.Enderby/Desktop/test.log");
        juce::FileOutputStream out (log);
        //out.setPosition (0);
        //out.truncate();
        
        out.writeString (juce::String(test) + "\n");
    }
};
