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
    
    // Initialise score counter;
    scoreCounter = 0;
}

//==============================================================================
void EnsembleModel::setSampleRate (double newSampleRate)
{
    sampleRate = newSampleRate;
}

void EnsembleModel::setTempo (double bpm)
{
    int newSamplesPerBeat = 60.0 * sampleRate / bpm;
    
    // Check tempo has actually changed.
    if (newSamplesPerBeat == samplesPerBeat)
    {
        return;
    }
    
    // Update tempo of playback.
    samplesPerBeat = newSamplesPerBeat;
    
    for (auto &player : players)
    {
        player->setOnsetInterval (samplesPerBeat);
    }
}

//==============================================================================
void EnsembleModel::processMidiBlock (juce::MidiBuffer &midi, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        for (auto &player : players)
        {
            player->processSample (midi, i);
        }
        
        if (newOnsetsAvailable())
        {
            calculateNewIntervals();
            clearOnsetsAvailable();
        }
        
        ++scoreCounter;
    }
}

//==============================================================================
bool EnsembleModel::newOnsetsAvailable()
{
    bool available = true;
    
    for (auto &player : players)
    {
        available = available && player->hasNotePlayed();
    }
    
    return available;
}

void EnsembleModel::calculateNewIntervals()
{
    for (int i = 0; i < players.size(); ++i)
    {
        double asyncSum = 0;
        
        for (int j = 0; j < players.size(); ++j)
        {
            double async = players [i]->getLatestOnsetTime() - players [j]->getLatestOnsetTime();
            asyncSum += alphas[i][j] * async;
        }
        
        int hNoise = players [i]->generateHNoise() * sampleRate;

        players [i]->setOnsetInterval (samplesPerBeat - asyncSum + hNoise);
    }
}

void EnsembleModel::clearOnsetsAvailable()
{
   for (auto &player : players)
    {
        player->resetNotePlayed();
    } 
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
    // Create a Player for each track in the file which has note on events.
    int nTracks = file.getNumTracks();
    
    for (int i = 0; i < nTracks; ++i)
    {
        auto track = file.getTrack (i);
        
        if (checkMidiSequenceHasNotes (track))
        {
            // Assing channels to players in a cyclical manner.
            int channelToUse = (i % 16) + 1;
            
            players.push_back (std::make_unique <Player> (track,
                                                          channelToUse,
                                                          sampleRate,
                                                          scoreCounter,
                                                          samplesPerBeat));
        }
    }
}

void EnsembleModel::initialiseAlphas()
{
    alphas.resize (players.size());
    
    for (auto &a : alphas)
    {
        a.resize (players.size(), 0.1);
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