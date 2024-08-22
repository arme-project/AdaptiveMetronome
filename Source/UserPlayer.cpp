#include "UserPlayer.h"
#include "PluginProcessor.h"

//==============================================================================
UserPlayer::UserPlayer (int index, const juce::MidiMessageSequence *seq, int midiChannel, 
                        const double &sampleRate, const int &scoreCounter, int initialInterval)
  : Player (index, seq, midiChannel, sampleRate, scoreCounter, initialInterval)
{
}

UserPlayer::UserPlayer (int index, const juce::MidiMessageSequence *seq, int midiChannel,
                        const double &sampleRate, const int &scoreCounter, int initialInterval, AdaptiveMetronomeAudioProcessor* processorIn)
  : Player (index, seq, midiChannel, sampleRate, scoreCounter, initialInterval, processorIn)
{
}

UserPlayer::~UserPlayer()
{
}

//==============================================================================
bool UserPlayer::isUserOperated()
{
    return true;
}

// New function signature
void UserPlayer::recalculateOnsetInterval (int samplesPerBeat,
                                           const std::vector <std::unique_ptr <Player> > &players)
{
    //==========================================================================
    // Find mean onset and interval for all other players
    float meanOnset = 0.0f;
    float meanInterval = 0.0f;
    int nOtherPlayers = 0;
    
    for (int i = 0; i < players.size(); ++i)
    {
        if (!players [i]->isUserOperated())
        {
            meanOnset += players [i]->getLatestOnsetTime();
            meanInterval += players [i]->getOnsetInterval();
            ++nOtherPlayers;
        }
    }
    
    meanOnset /= nOtherPlayers;
    meanInterval /= nOtherPlayers;
    
    //==========================================================================
    // Calculate next onset interval so that a note will be played automatically
    // between the next two non-user player onset times. If there are only human
    // players, just use the most recently played interval.
    if (nOtherPlayers == 0)
    {
        // TODO: potentially do something clever here
    }
    else
    {
        onsetInterval = meanOnset - currentOnsetTime + 1.5 * meanInterval;
    }
}

//==============================================================================
bool UserPlayer::wasLatestOnsetUserInput()
{
    return noteTriggeredByUser;
}


void UserPlayer::processIntroSample (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex, int introNote)
{
    int channelParam = processor->channelParameter(playerIndex)->get();
    juce::uint8 velocity = 100;

    if (samplesToIntroToneOff == 0)
    {
        outMidi.addEvent (juce::MidiMessage::noteOff (channelParam, introNote, velocity),
                       sampleIndex);
    }
    
    // Loop through samples and check for a noteOn event
    for (auto it = inMidi.findNextSamplePosition (sampleIndex); it != inMidi.end(); ++it)
    {
        auto event = (*it).getMessage();
        
        // Play the next note at the first note on in this beat period
        if (event.isNoteOn())
        {
            // Check if incomming note is on the midi channel associated with this player (channelParam)
            if (event.getChannel() == channelParam) {
                
                outMidi.addEvent (juce::MidiMessage::noteOn (channelParam, introNote, velocity),
                                   sampleIndex);
                samplesToIntroToneOff = introToneLength;
            }
        }
    } // Finish sample loop
    samplesToIntroToneOff--;
}


//==============================================================================
void UserPlayer::processNoteOn (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex)
{
    // If first sample in score ... play the first note automatically
     if (scoreCounter == 0)
     {
         noteTriggeredByUser = true;
         playNextNote (outMidi, sampleIndex);
         return;
     }
    
    // Loop over all MIDI events at this sample position
    for (auto it = inMidi.findNextSamplePosition (sampleIndex); it != inMidi.end(); ++it)
    {
        // Filters ... ignore if following conditions are true ....
        
        // Ignore if score has not progressed half an interval length
        if (scoreCounter <= (onsetInterval / 2))
        {
           break;
        }
        
        // Ignore If previous note has not yet been processed
        if (notePlayed) {
            break;
        }
        
        // Ignore If time since last onset is less than 50% of onset interval
        if (samplesSinceLastOnset < onsetInterval/2) {
            break;
        }
        
        // Stop early if there are MIDI events after this time.
        if ((*it).samplePosition != sampleIndex)
        {
            break;
        }
        
        // If filters have passed ... continue here
        auto event = (*it).getMessage();
        
        // Play the next note at the first note on in this beat period
        if (event.isNoteOn() && !notePlayed)
        {
            // Check if incomming note is on the midi channel associated with this player (channelParam)
            int channelParam = processor->channelParameter(playerIndex)->get();
            if (event.getChannel() == channelParam) {
                noteTriggeredByUser = true;
                playNextNote (outMidi, sampleIndex);
            }
        }
    } // Finish sample loop
    
    // If no user input trigger a note automatically if no note has been played for 50% of interval time
    if (!notePlayed && (samplesSinceLastOnset >= (onsetInterval * 1.5)))
    {
        noteTriggeredByUser = false;
        playNextNote (outMidi, sampleIndex);
    }
}
