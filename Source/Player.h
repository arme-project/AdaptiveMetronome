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
    Player (const juce::MidiMessageSequence *seq);
    ~Player();
    
    //==============================================================================
    void reset();
    
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
        int noteNumber;
        juce::uint8 velocity;
        double duration;
    };
    
    std::vector <Note> notes;
    std::size_t currentNoteIndex;
    
    void initialiseScore (const juce::MidiMessageSequence *seq);
    
    //==============================================================================
    // Timing information
    int onsetInterval;
    std::size_t nextOnsetTime;
};