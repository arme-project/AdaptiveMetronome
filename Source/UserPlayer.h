#pragma once
#include "Player.h"

class UserPlayer : public Player
{
public:
    //==============================================================================
    UserPlayer (int index, const juce::MidiMessageSequence *seq, int midiChannel, 
                const double &sampleRate, const int &scoreCounter, int initialInterval);
    ~UserPlayer();
    
    //==============================================================================
    bool isUserOperated() override;
    
    //==============================================================================
    void recalculateOnsetInterval (int samplesPerBeat,
                                   const std::vector <std::unique_ptr <Player> > &players,
                                   const std::vector <std::unique_ptr <juce::AudioParameterFloat> > &alphas) override;
                                   
    //==============================================================================
    bool wasLatestOnsetUserInput() override;
                                           
protected:
    //==============================================================================
    bool noteTriggeredByUser = false;
    
    //==============================================================================
    void processNoteOn (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex) override;

private:
};
