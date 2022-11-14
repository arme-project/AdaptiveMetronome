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
    
                
    // Add details of most recent onsets to buffers to be logged.
    if (loggingFifo)
    {
        auto writer = loggingFifo->write (players.size());

        int p = 0;
        
        for (int i = 0; i < writer.blockSize1; ++i)
        {
            storeOnsetDetailsForPlayer (writer.startIndex1 + i, p++);
        }
        
        for (int i = 0; i < writer.blockSize2; ++i)
        {
            storeOnsetDetailsForPlayer (writer.startIndex2 + i, p++);
        }
    } 
}

void EnsembleModel::clearOnsetsAvailable()
{
   for (auto &player : players)
    {
        player->resetNotePlayed();
    } 
}

void EnsembleModel::storeOnsetDetailsForPlayer (int bufferIndex, int playerIndex)
{
    onsetBuffer [bufferIndex] = players [playerIndex]->getLatestOnsetTime();
    onsetIntervalBuffer [bufferIndex] = players [playerIndex]->getOnsetInterval();
    motorNoiseBuffer [bufferIndex] = players [playerIndex]->getMotorNoise();
    timeKeeperNoiseBuffer [bufferIndex] = players [playerIndex]->getTimeKeeperNoise();
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
void EnsembleModel::initialiseLoggingBuffers (int bufferSize)
{
    loggingFifo = std::make_unique <juce::AbstractFifo> (bufferSize);
    onsetBuffer.resize (bufferSize, 0);
    onsetIntervalBuffer.resize (bufferSize, 0);
    motorNoiseBuffer.resize (bufferSize, 0.0);
    timeKeeperNoiseBuffer.resize (bufferSize, 0.0);
}

void EnsembleModel::startLoggerLoop()
{
    stopLoggerLoop();
    initialiseLoggingBuffers (4 * players.size());
        
    continueLogging = true;
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
    juce::File logFile ("/Users/Sean.Enderby/Desktop/test.csv");
    juce::FileOutputStream logStream (logFile);
    logStream.setPosition (0);
    logStream.truncate();
    
    logLineCounter = 0;
    
    while (continueLogging)
    {
        logOnsetDetails (logStream);
        std::this_thread::sleep_for (50ms);
    }
}

void EnsembleModel::logOnsetDetails (juce::FileOutputStream &logStream)
{
    while (loggingFifo->getNumReady() > 0)
    {
        std::vector <int> latestOnsets (players.size());
        juce::String logLine (logLineCounter++);
        juce::String onsetLog, intervalLog, mNoiseLog, tkNoiseLog;
        int p = 0;

        auto reader = loggingFifo->read (players.size());
        
        for (int i = 0; i < reader.blockSize1; ++i)
        {
            int onsetTime = onsetBuffer [reader.startIndex1 + i];
            latestOnsets [p++] = onsetTime;
            
            logOnsetDetailsForPlayer (reader.startIndex1 + i,
                                      onsetLog,
                                      intervalLog,
                                      mNoiseLog,
                                      tkNoiseLog);                 
        }
        
        for (int i = 0; i < reader.blockSize2; ++i)
        {
            int onsetTime = onsetBuffer [reader.startIndex2 + i];
            latestOnsets [p++] = onsetTime;
     
            logOnsetDetailsForPlayer (reader.startIndex2 + i,
                                      onsetLog,
                                      intervalLog,
                                      mNoiseLog,
                                      tkNoiseLog);
        }
        
        logLine += onsetLog + ", " + 
                   intervalLog + ", " + 
                   mNoiseLog + ", " +
                   tkNoiseLog + "\n";
                   
        logStream.writeText (logLine, false, false, nullptr);
        
        //==========================================================================
        // At this point latestOnsets contains the onset time in samples for
        // each of the players' most recently played notes. Do what you will
        // with them here.
    }
}

void EnsembleModel::logOnsetDetailsForPlayer (int bufferIndex,
                                              juce::String &onsetLog,
                                              juce::String &intervalLog,
                                              juce::String &mNoiseLog,
                                              juce::String &tkNoiseLog)
{
    onsetLog += ", " + juce::String (onsetBuffer [bufferIndex] / sampleRate);
    intervalLog += ", " + juce::String (onsetIntervalBuffer [bufferIndex] / sampleRate);
    mNoiseLog += ", " + juce::String (motorNoiseBuffer [bufferIndex]);
    tkNoiseLog += ", " + juce::String (timeKeeperNoiseBuffer [bufferIndex]);
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
