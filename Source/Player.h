#pragma once
#include <JuceHeader.h>
#include <vector>
#include <random>

/**
 * A class for playing back a sequence of MIDI note on/off events at given intervals.
 */
class Player
{
public:
    //==============================================================================
    Player (const juce::MidiMessageSequence *seq, int midiChannel, 
            const double &sampleRate, const int &scoreCounter, int initialInterval);
    ~Player();
    
    //==============================================================================
    void reset();
    
    //==============================================================================
    void setOnsetInterval (int interval);
    
    //==============================================================================
    double generateMotorNoise();
    double generateTimeKeeperNoise();
    double generateHNoise();
    
    //==============================================================================
    bool hasNotePlayed();
    void resetNotePlayed();
    
    int getLatestOnsetTime();
    
    //==============================================================================
    void processSample (juce::MidiBuffer &midi, int sampleIndex);
    
    //==============================================================================
    std::size_t getNumNotes();
    
private:    
    //==============================================================================
    // MIDI config
    int channel;

    //==============================================================================
    // Score information
    struct Note
    {
        int noteNumber; // MIDI note number
        juce::uint8 velocity; // MIDI velocity
        double duration; // Note duration in seconds
    };
    
    std::vector <Note> notes;
    std::size_t currentNoteIndex = 0;
    
    void initialiseScore (const juce::MidiMessageSequence *seq);
    void playNextNote (juce::MidiBuffer &midi, int sampleIndex);
    void stopPreviousNote (juce::MidiBuffer &midi, int sampleIndex);
    
    //==============================================================================
    // Timing information
    const double &sampleRate;
    const int &scoreCounter;
    int onsetInterval; // time between previous and next onset in samples
    
    int samplesSinceLastOnset = 0, samplesToNextOffset = -1;
    
    int currentOnsetTime = 0, previousOnsetTime = 0;
    bool notePlayed = false;
    
    //==============================================================================
    // Randomness
    static std::random_device randomSeed;
    static std::default_random_engine randomEngine;
    std::normal_distribution <double> mNoiseDistribution, tkNoiseDistribution;
    
    double currentMotorNoise = 0.0, previousMotorNoise = 0.0;
};
