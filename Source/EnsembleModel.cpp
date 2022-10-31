#include "EnsembleModel.h"

//==============================================================================
EnsembleModel::EnsembleModel()
{
}

EnsembleModel::~EnsembleModel()
{
}

//==============================================================================
void EnsembleModel::loadMidiFile (const juce::File &file)
{
    //==========================================================================
    // Read in content of MIDI file.
    juce::FileInputStream inStream (file);
    
    if (!inStream.openedOk())
        return; // put some error handling here
        
    int fileType = 0;
    
    juce::MidiFile midiFile;
    
    if (!midiFile.readFrom (inStream, true, &fileType))
        return; // more error handling
        
    midiFile.convertTimestampTicksToSeconds();
    
    //==========================================================================
    // Create player for each track in the file.
    clearPlayers(); // delete old players
    createPlayers (midiFile); // create new players
    initialiseAlphas(); // allocate alpha matrix
    
    
    //==========================================================================
    debugLog();
}

//==============================================================================
void EnsembleModel::clearPlayers()
{
    // Remove anything to do with old players.
    players.clear();
    alphas.clear();
}

void EnsembleModel::createPlayers (const juce::MidiFile &file)
{
    int nTracks = file.getNumTracks();
    
    for (int i = 0; i < nTracks; ++i)
    {
        auto track = file.getTrack (i);
        
        if (checkMidiSequenceHasNotes (track))
        {
            players.push_back (std::make_unique <Player> (track));
        }
    }
}

void EnsembleModel::initialiseAlphas()
{
    alphas.resize (players.size());
    
    for (auto &a : alphas)
    {
        a.resize (players.size(), 0.0);
    }
}

//==============================================================================
bool EnsembleModel::checkMidiSequenceHasNotes (const juce::MidiMessageSequence *seq)
{
    for (auto event : *seq)
    {
        if (event->message.isNoteOn())
        {
            return true;
        }
    }
    
    return false;
}