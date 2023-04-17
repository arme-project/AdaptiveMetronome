#include "UserPlayer.h"
#include "PluginEditor.h"

using Editor = AdaptiveMetronomeAudioProcessorEditor;

//==============================================================================
UserPlayer::UserPlayer (int index, const juce::MidiMessageSequence *seq, int midiChannel, 
                        const double &sampleRate, const int &scoreCounter, int initialInterval)
  : Player (index, seq, midiChannel, sampleRate, scoreCounter, initialInterval)
{
    useOSCinput = true;
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

    // Calculate latest onsetInterval in seconds

    //double onsetIntervalInSamples = getPlayedOnsetIntervalInSamples();
    //double onsetIntervalInSeconds = onsetIntervalInSamples/sampleRate;
    //addIntervalToQueue(onsetIntervalInSeconds);

    float meanOnset = 0.0f;
    float meanInterval = 0.0f;
    int nOtherPlayers = 0;
    
    for (int i = 0; i < players.size(); ++i)
    {
        if (!players [i]->isUserOperated())
        {
            meanOnset += players [i]->getLatestOnsetTimeInSamples();
            meanInterval += players [i]->getOnsetIntervalForNextNote();
            ++nOtherPlayers;
        }
    }
    
    meanOnset /= nOtherPlayers;
    meanInterval /= nOtherPlayers;
    
    //==========================================================================
    // Calculate next onset interval so that a note will be played automatically
    // between the next two non-user player onset times.
    //onsetIntervalForNextNote = meanOnset - currentOnsetTimeInSamples + 1.5 * meanInterval;
    onsetIntervalForNextNote = meanOnset - currentOnsetTimeInSamples + meanInterval;
    if (logToLogger) {
        juce::String message;
        message << "Player: " << playerIndex << " : " << onsetIntervalForNextNote;
        Editor::writeToLogger(system_clock::now(), "Player", "recalculateOnsetInterval", message);
    }
}

//==============================================================================
bool UserPlayer::wasLatestOnsetUserInput()
{
    return noteTriggeredByUser;
}

juce::String UserPlayer::getNoteTriggeredByUser()
{
    if (noteTriggeredByUser) {
        return "yes";
    }
    else {
        return "no";
    }
}

//==============================================================================
void UserPlayer::processNoteOn (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex)
{
    if (useOSCinput) {
        if (newOSCOnsetAvailable) {
            noteTriggeredByUser = true;
            userPlayedNote = true;
            newOSCOnsetAvailable = false;
            playNextNote(outMidi, sampleIndex);
        }
    }
    else {
        // Loop over all MIDI events at this sample position
        for (auto it = inMidi.findNextSamplePosition (sampleIndex); it != inMidi.end(); ++it)
        {
            // Stop early if there are MIDI events after this time.
            if ((*it).samplePosition != sampleIndex)
            {
                break;
            }
        
            auto event = (*it).getMessage();
        
            //if (event.isNoteOn()) {
            //    DBG("NOTE IS SO ON");
            //}
            // 
            // Play the next note at the first note on in this beat period
            if (event.isNoteOn() && !notePlayed && scoreCounter > (onsetIntervalForNextNote / 2)) {
                playNextNote (outMidi, sampleIndex);
                noteTriggeredByUser = true;
                userPlayedNote = true;
            }
        }
    }
    
    //// If no user input trigger a note automatically
    //if (!notePlayed && (samplesSinceLastOnset >= onsetInterval || scoreCounter == 0))
    //{
    //    playNextNote (outMidi, sampleIndex);
    //    noteTriggeredByUser = false;
    //}
}
