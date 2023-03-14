#include "UserPlayer.h"

//==============================================================================
UserPlayer::UserPlayer (int index, const juce::MidiMessageSequence *seq, int midiChannel, 
                        const double &sampleRate, const int &scoreCounter, int initialInterval)
  : Player (index, seq, midiChannel, sampleRate, scoreCounter, initialInterval)
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

//==============================================================================
void UserPlayer::recalculateOnsetInterval (int samplesPerBeat,
                                           const std::vector <std::unique_ptr <Player> > &players,
                                           const std::vector <std::unique_ptr <juce::AudioParameterFloat> > &alphas)
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
    // between the next two non-user player onset times.
    onsetInterval = meanOnset - currentOnsetTime + 1.5 * meanInterval;
}

//==============================================================================
bool UserPlayer::wasLatestOnsetUserInput()
{
    return noteTriggeredByUser;
}

//==============================================================================
void UserPlayer::processNoteOn (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex)
{
    // Loop over all MIDI events at this sample position
    for (auto it = inMidi.findNextSamplePosition (sampleIndex); it != inMidi.end(); ++it)
    {
        // Stop early if there are MIDI events after this time.
        if ((*it).samplePosition != sampleIndex)
        {
            break;
        }
        
        auto event = (*it).getMessage();
        
        // Play the next note at the first note on in this beat period
        if (event.isNoteOn() && !notePlayed && scoreCounter > (onsetInterval / 2))
        {
            playNextNote (outMidi, sampleIndex);
            noteTriggeredByUser = true;
            DBG("NOTE PLAYED BY USER");
        }
    }
    
    // If no user input trigger a note automatically
    if (!notePlayed && (samplesSinceLastOnset >= onsetInterval || scoreCounter == 0))
    {
        playNextNote (outMidi, sampleIndex);
        noteTriggeredByUser = false;
    }
}
