#pragma once
#include <JuceHeader.h>
#include <vector>

/**
 * A class for playing back a sequence of MIDI note on/off events at given intervals.
 */
class Player
{
public:
    //==============================================================================
    Player (const juce::MidiMessageSequence *seq, int midiChannel, 
            double sampleRate, int initialInterval);
    ~Player();
    
    //==============================================================================
    void reset();
    
    //==============================================================================
    void setSampleRate (double newSampleRate);
    void setOnsetInterval (int interval);
    
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
    std::size_t currentNoteIndex;
    
    void initialiseScore (const juce::MidiMessageSequence *seq);
    
    //==============================================================================
    // Timing information
    double sampleRate;
    int onsetInterval; // time between previous and next onset in samples
    int samplesSinceLastOnset;
    int samplesToNextOffset;
};