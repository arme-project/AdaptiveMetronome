#include "EnsembleModel.h"

using namespace std::chrono_literals;

//==============================================================================
EnsembleModel::EnsembleModel()
{
    playersInUse.clear();
}

EnsembleModel::~EnsembleModel()
{
    stopLoggerLoop();
}

//==============================================================================
bool EnsembleModel::loadMidiFile (const juce::File &file)
{
    FlagLock lock (playersInUse);
    
    if (!lock.locked)
    {
        return false;
    }
    
    //==========================================================================
    // Read in content of MIDI file.
    juce::FileInputStream inStream (file);
    
    if (!inStream.openedOk())
        return false; // put some error handling here
        
    int fileType = 0;
    
    juce::MidiFile midiFile;
    
    if (!midiFile.readFrom (inStream, true, &fileType))
        return false; // more error handling
        
    midiFile.convertTimestampTicksToSeconds();
    
    //==========================================================================
    // Create player for each track in the file.
    clearPlayers(); // delete old players
    createPlayers (midiFile); // create new players
    initialiseAlphas(); // allocate alpha matrix
    
    // Initialise score counter
    scoreCounter = 0;
    
    // Start loop for logging onset times or each player
    startLoggerLoop();
    
    return true;
}

//==============================================================================
void EnsembleModel::setSampleRate (double newSampleRate)
{
    sampleRate = newSampleRate;
}

//==============================================================================
void EnsembleModel::processMidiBlock (juce::MidiBuffer &midi, int numSamples, double tempo)
{
    FlagLock lock (playersInUse);
    
    if (!lock.locked)
    {
        return;
    }
    
    setTempo (tempo);
    
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
    
    setInitialPlayerTempo();
}

void EnsembleModel::setInitialPlayerTempo()
{
    if (!initialTempoSet)
    {
        for (auto &player : players)
        {
            player->setOnsetInterval (samplesPerBeat);
        }
        
        initialTempoSet = true;
    }
}

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
            
    // Put most recent onset times in buffer to be logged.
    if (onsetFifo)
    {
        auto writer = onsetFifo->write (players.size());

        int p = 0;
        
        for (int i = 0; i < writer.blockSize1; ++i)
        {
            onsetBuffer [writer.startIndex1 + i] = players [p++]->getLatestOnsetTime();
        }
        
        for (int i = 0; i < writer.blockSize2; ++i)
        {
            onsetBuffer [writer.startIndex2 + i] = players [p++]->getLatestOnsetTime();
        }
    }
    
    // Calculate new onset times for players.
    for (int i = 0; i < players.size(); ++i)
    {
        double asyncSum = 0;
        
        for (int j = 0; j < players.size(); ++j)
        {
            double async = players [i]->getLatestOnsetTime() - players [j]->getLatestOnsetTime();
            asyncSum += alphas[i][j] * async;
        }
        
        double hNoise = players [i]->generateHNoise() * sampleRate;

        players [i]->setOnsetInterval (samplesPerBeat - (asyncSum + hNoise));
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
EnsembleModel::FlagLock::FlagLock (std::atomic_flag &f)
  : flag (f),
    locked (!flag.test_and_set())
{
}

EnsembleModel::FlagLock::~FlagLock()
{
    flag.clear();
}

//==============================================================================
void EnsembleModel::clearPlayers()
{
    // Remove anything to do with old players.
    players.clear();
    alphas.clear();
    initialTempoSet = false;
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
void EnsembleModel::initialiseOnsetBuffer (int bufferSize)
{
    onsetFifo = std::make_unique <juce::AbstractFifo> (bufferSize);
    onsetBuffer.resize (bufferSize, 0);
}

void EnsembleModel::startLoggerLoop()
{
    stopLoggerLoop();
    initialiseOnsetBuffer (4 * players.size());
    loggerThread = std::thread ([this] () {this->loggerLoop();});
}

void EnsembleModel::stopLoggerLoop()
{
    continueLogging = false;
    
    if (loggerThread.joinable())
    {
        loggerThread.join();
    }
}

void EnsembleModel::loggerLoop()
{
    juce::File logFile ("/Users/Sean.Enderby/Desktop/test.log");
    juce::FileOutputStream logStream (logFile);
    logStream.setPosition (0);
    logStream.truncate();
    
    continueLogging = true;
    
    while (continueLogging)
    {
        logOnsets (logStream);
        std::this_thread::sleep_for (500ms);
    }
}

void EnsembleModel::logOnsets (juce::FileOutputStream &logStream)
{
    while (onsetFifo->getNumReady() > 0)
    {
        std::vector <int> latestOnsets (players.size());
        juce::String logLine ("Onsets: ");
        int p = 0;

        auto reader = onsetFifo->read (players.size());
        
        for (int i = 0; i < reader.blockSize1; ++i)
        {
            int onsetTime = onsetBuffer [reader.startIndex1 + i];
            latestOnsets [p++] = onsetTime;
            logLine += juce::String (onsetTime) + ", ";
        }
        
        for (int i = 0; i < reader.blockSize2; ++i)
        {
            int onsetTime = onsetBuffer [reader.startIndex2 + i];
            latestOnsets [p++] = onsetTime;
            logLine += juce::String (onsetTime) + ", ";
        }
        
        logStream.writeText (logLine + "\n", false, false, nullptr);
        
        //==========================================================================
        // At this point latestOnsets contains the onset time in samples for
        // each of the players' most recently played notes. Do what you will
        // with them here.
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
