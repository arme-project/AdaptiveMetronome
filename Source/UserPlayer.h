#pragma once
#include "Player.h"

class UserPlayer : public Player
{
public:
    //==============================================================================
    UserPlayer (int index, const juce::MidiMessageSequence *seq, int midiChannel, 
                const double &sampleRate, const int &scoreCounter, int initialInterval);
    UserPlayer (int index, const juce::MidiMessageSequence *seq, int midiChannel,
                const double &sampleRate, const int &scoreCounter, int initialInterval, AdaptiveMetronomeAudioProcessor *processorIn);
    ~UserPlayer();
    
    //==============================================================================
    bool isUserOperated() override;
    
    //==============================================================================
    void recalculateOnsetInterval (int samplesPerBeat,
                                   const std::vector <std::unique_ptr <Player> > &players) override;
    
    //==============================================================================
    bool wasLatestOnsetUserInput() override;
                  
    void processIntroSample (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex, int introNote) override;

protected:
    //==============================================================================
    // bool noteTriggeredByUser = false;
    
    //==============================================================================
    void processNoteOn (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex) override;

    int introToneLength = 100;
    int samplesToIntroToneOff = -1;
private:
};
