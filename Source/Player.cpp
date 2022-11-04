#include "Player.h"

//==============================================================================
Player::Player (const juce::MidiMessageSequence *seq, int midiChannel, 
                double sampleRate, int initialInterval)
  : channel (midiChannel),
    sampleRate (sampleRate),
    onsetInterval (initialInterval),
    samplesSinceLastOnset (0),
    samplesToNextOffset (-1)
{
    initialiseScore (seq);
}

Player::~Player()
{
}

//==============================================================================
void Player::reset()
{
    // rewind to start of score
    currentNoteIndex = 0;
}

//==============================================================================
void Player::setSampleRate (double newSampleRate)
{
    sampleRate = newSampleRate;
}

void Player::setOnsetInterval (int interval)
{
    onsetInterval = interval;
}

//==============================================================================
void Player::processSample (juce::MidiBuffer &midi, int sampleIndex)
{
    if (currentNoteIndex >= notes.size())
    {
        return;
    }
    
    if (samplesSinceLastOnset == onsetInterval)
    {
        auto &note = notes [++currentNoteIndex];
        
        midi.addEvent (juce::MidiMessage::noteOn (channel, note.noteNumber, note.velocity),
                       sampleIndex);
                       
        samplesSinceLastOnset = 0;
        
        samplesToNextOffset = note.duration * sampleRate;
    }
    
    if (samplesToNextOffset == 0)
    {
        auto &note = notes [currentNoteIndex - 1];
        
        midi.addEvent (juce::MidiMessage::noteOff (channel, note.noteNumber, note.velocity),
                       sampleIndex);
    }
    
    ++samplesSinceLastOnset;
    
    if (samplesToNextOffset >= 0)
    {
        --samplesToNextOffset;
    }
}

//==============================================================================
std::size_t Player::getNumNotes()
{
    return notes.size();
}

//==============================================================================
void Player::initialiseScore (const juce::MidiMessageSequence *seq)
{
    // Get all note events from MIDI sequence.
    // At the moment note on times are discarded and the notes will be
    // played back in sequence according to timing set by the EnsembleModel.
    // The duration for each note is saved as the time between note on
    // and note off in the MIDI file.
    for (auto event : *seq)
    {
        auto message = event->message;
        
        if (message.isNoteOn())
        {
            auto noteOff = event->noteOffObject->message;
            
            Note note {message.getNoteNumber(),
                       message.getVelocity(),
                       noteOff.getTimeStamp() - message.getTimeStamp()};
                       
            notes.push_back (note);
        }
    }
    
    // Reset to start of score.
    reset();
}