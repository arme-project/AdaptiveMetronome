#include "EnsembleModel.h"
#include "UserPlayer.h"

using namespace std::chrono_literals;

//==============================================================================
EnsembleModel::EnsembleModel()
{
    playersInUse.clear();
    resetFlag.clear();
}

EnsembleModel::~EnsembleModel()
{
    stopLoggerLoop();
    stopPollingLoop();
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
    createPlayers (midiFile); // create new players
    resetPlayers();
    
    return true;
}

bool EnsembleModel::reset()
{
    FlagLock lock (playersInUse);
    
    if (!lock.locked)
    {
        return false;
    }
    
    resetPlayers();
    
    return true;
}

//==============================================================================
void EnsembleModel::prepareToPlay (double newSampleRate)
{
    sampleRate = newSampleRate;
}

void EnsembleModel::releaseResources()
{
}

//==============================================================================
void EnsembleModel::processMidiBlock (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int numSamples, double tempo)
{
    FlagLock lock (playersInUse);
    
    if (!lock.locked)
    {
        return;
    }
    
    //==============================================================================
    // Update tempo from DAW playhead.
    setTempo (tempo);
    
    //==============================================================================
    // Clear output if ensemble has been reset
    if (!resetFlag.test_and_set())
    {
        soundOffAllChannels (outMidi);
    }
    
    //==============================================================================
    // Process each sample of the buffer for each player.
    for (int i = 0; i < numSamples; ++i)
    {
        if (introTonesPlayed < numIntroTones)
        {
            playIntroTones (outMidi, i);
            continue;
        }
        
        playScore (inMidi, outMidi, i);
    }
}

//==============================================================================
int EnsembleModel::getNumPlayers()
{
    return static_cast <int> (players.size());
}

bool EnsembleModel::isPlayerUserOperated (int playerIndex)
{
    return players [playerIndex]->isUserOperated();
}

juce::AudioParameterInt& EnsembleModel::getPlayerChannelParameter (int playerIndex)
{
    return players [playerIndex]->channelParam;
}

juce::AudioParameterFloat& EnsembleModel::getPlayerMotorNoiseParameter (int playerIndex)
{
    return players [playerIndex]->mNoiseStdParam;
}

juce::AudioParameterFloat& EnsembleModel::getPlayerTimeKeeperNoiseParameter (int playerIndex)
{
    return players [playerIndex]->tkNoiseStdParam;
}

juce::AudioParameterFloat& EnsembleModel::getPlayerVolumeParameter (int playerIndex)
{
    return players [playerIndex]->volumeParam;
}

juce::AudioParameterFloat& EnsembleModel::getAlphaParameter (int player1Index, int player2Index)
{
    return *alphaParams [player1Index][player2Index];
}

//==============================================================================
void EnsembleModel::soundOffAllChannels (juce::MidiBuffer &midi)
{
    for (int channel = 1; channel <= 16; ++channel)
    {
        midi.addEvent (juce::MidiMessage::allNotesOff (channel), 0);
        midi.addEvent (juce::MidiMessage::allSoundOff (channel), 0);
        midi.addEvent (juce::MidiMessage::allControllersOff (channel), 0);
    }
}

//==============================================================================
void EnsembleModel::playIntroTones (juce::MidiBuffer &midi, int sampleIndex)
{
    if (introCounter == 0)
    {
        introToneOn (midi, sampleIndex);
    }
    else if (introCounter == samplesPerBeat / 4)
    {
        introToneOff (midi, sampleIndex);
    }
    else if (introCounter >= samplesPerBeat - 1)
    {
        ++introTonesPlayed;
        introCounter = -1;
    }
    
    ++introCounter;
}

void EnsembleModel::introToneOn (juce::MidiBuffer &midi, int sampleIndex)
{
    midi.addEvent (juce::MidiMessage::noteOn (1, introToneNote, introToneVel), sampleIndex);
}

void EnsembleModel::introToneOff (juce::MidiBuffer &midi, int sampleIndex)
{
    midi.addEvent (juce::MidiMessage::noteOff (1, introToneNote, introToneVel), sampleIndex);
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
    //==========================================================================
    // Get most recent alphas
    getLatestAlphas();
    
    //==========================================================================
    // Calculate new onset times for players.
    // Make sure all non-user players update before the user players.
    for (int i = 0; i < players.size(); ++i)
    {
        if (!players [i]->isUserOperated())
        {
            players [i]->recalculateOnsetInterval (samplesPerBeat, players, alphaParams [i]);
        }
    }  
    
    for (int i = 0; i < players.size(); ++i)
    {
        if (players [i]->isUserOperated())
        {
            players [i]->recalculateOnsetInterval (samplesPerBeat, players, alphaParams [i]);
        }
    } 
          
    //==========================================================================
    // Add details of most recent onsets to buffers to be logged.
    if (loggingFifo)
    {
        auto writer = loggingFifo->write (static_cast <int> (players.size()));

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

void EnsembleModel::getLatestAlphas()
{
    if (pollingFifo)
    {
        // Consume everything in the buffer, only using the most recent set of alphas.
        auto reader = pollingFifo->read (pollingFifo->getNumReady()); 
        
        for (int player1 = 0; player1 < pollingBuffer.size(); ++player1)
        {
            int player2 = 0;
            
            int block1Start = std::max (reader.blockSize1 + reader.blockSize2 - static_cast <int> (players.size()), 0);

            for (int i = block1Start; i < reader.blockSize1; ++i)
            {
                *alphaParams [player1][player2++] = pollingBuffer [player1][reader.startIndex1 + i];
            }
            
            int block2Start = std::max (block1Start - reader.blockSize1, 0);
        
            for (int i = block2Start; i < reader.blockSize2; ++i)
            {
                *alphaParams [player1][player2++] = pollingBuffer [player1][reader.startIndex2 + i];
            }
        }
    } 
}

void EnsembleModel::storeOnsetDetailsForPlayer (int bufferIndex, int playerIndex)
{
    // Store the log information about the latest onset from the given player
    // in the logging buffers.
    onsetBuffer [bufferIndex] = players [playerIndex]->getLatestOnsetTime();
    onsetIntervalBuffer [bufferIndex] = players [playerIndex]->getPlayedOnsetInterval();
    motorNoiseBuffer [bufferIndex] = players [playerIndex]->getMotorNoise();
    timeKeeperNoiseBuffer [bufferIndex] = players [playerIndex]->getTimeKeeperNoise();
    
    for (int i = 0; i < players.size(); ++i)
    {
        asyncBuffer [i][bufferIndex] = players [playerIndex]->getLatestOnsetTime() - players [i]->getLatestOnsetTime();
        alphaBuffer [i][bufferIndex] = *alphaParams [playerIndex][i];
    }
    
    tkNoiseStdBuffer [bufferIndex] = players [playerIndex]->getTimeKeeperNoiseStd();
    mNoiseStdBuffer [bufferIndex] = players [playerIndex]->getMotorNoiseStd();
    volumeBuffer [bufferIndex] = players [playerIndex]->getLatestVolume();
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
void EnsembleModel::createPlayers (const juce::MidiFile &file)
{
    //==========================================================================
    // Delete Old Players
    players.clear();
    
    //==========================================================================
    // Create a Player for each track in the file which has note on events.
    int nTracks = file.getNumTracks();
    int playerIndex = 0;
    
    for (int i = 0; i < nTracks; ++i)
    {
        auto track = file.getTrack (i);
        
        if (checkMidiSequenceHasNotes (track))
        {
            // Assing channels to players in a cyclical manner.
            int channelToUse = (playerIndex % 16) + 1;
            
            if (playerIndex == 0)
            {
                players.push_back (std::make_unique <UserPlayer> (playerIndex++,
                                                                  track,
                                                                  channelToUse,
                                                                  sampleRate,
                                                                  scoreCounter,
                                                                  samplesPerBeat));
            }
            else
            {
                players.push_back (std::make_unique <Player> (playerIndex++,
                                                              track,
                                                              channelToUse,
                                                              sampleRate,
                                                              scoreCounter,
                                                              samplesPerBeat));
            }
        }
    }
    
    //==========================================================================
    createAlphaParameters(); // create matrix of parameters for alphas
}

void EnsembleModel::createAlphaParameters()
{
    alphaParams.clear();
    
    for (int i = 0; i < players.size(); ++i)
    {
        std::vector <std::unique_ptr <juce::AudioParameterFloat> > row;
        double alpha = 0.25;
        
        for (int j = 0; j < players.size(); ++j)
        {
            row.push_back (std::make_unique <juce::AudioParameterFloat> ("alpha-" + juce::String (i) + "-" + juce::String (j),
                                                                         "Alpha " + juce::String (i) + "-" + juce::String (j),
                                                                         0.0, 1.0, alpha));
                                                                         
            alpha = 0.0;
        }
        
        alphaParams.push_back (std::move (row));
    }
}

void EnsembleModel::playScore (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex)
{          
    for (auto &player : players)
    {
        player->processSample (inMidi, outMidi, sampleIndex);
    }
        
    // If all players have played a note, update timings.
    if (newOnsetsAvailable())
    {
        calculateNewIntervals();
        clearOnsetsAvailable();
    }
        
    ++scoreCounter;
}

void EnsembleModel::resetPlayers()
{
    //==========================================================================
    // Initialise intro countdown
    introCounter = -sampleRate / 2;
    introTonesPlayed = 0;
     
    // Initialise score counter
    scoreCounter = 0;
    
    // make sure to update player tempo when playback starts      
    initialTempoSet = false;
            
    // Start loop for logging onset times or each player
    startLoggerLoop();
    
    // Start loop which polls for new alpha values
    startPollingLoop();
    
    //==========================================================================
    // reset all players
    for (auto &player : players)
    {
        player->reset();
    }
    
    resetFlag.clear();
}

//==============================================================================
void EnsembleModel::initialiseLoggingBuffers()
{
    int bufferSize = static_cast <int> (4 * players.size());
    
    loggingFifo = std::make_unique <juce::AbstractFifo> (bufferSize);
    onsetBuffer.resize (bufferSize, 0);
    onsetIntervalBuffer.resize (bufferSize, 0);
    motorNoiseBuffer.resize (bufferSize, 0.0);
    timeKeeperNoiseBuffer.resize (bufferSize, 0.0);
    
    asyncBuffer.resize (players.size());
    alphaBuffer.resize (players.size());
    
    for (int i = 0; i < asyncBuffer.size(); ++i)
    {
        asyncBuffer [i].resize (bufferSize, 0.0);
        alphaBuffer [i].resize (bufferSize, 0.0);
    }
    
    tkNoiseStdBuffer.resize (bufferSize, 0.0);
    mNoiseStdBuffer.resize (bufferSize, 0.0);
    volumeBuffer.resize (bufferSize, 0.0);
}

void EnsembleModel::startLoggerLoop()
{
    stopLoggerLoop();
    initialiseLoggingBuffers();
        
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
    //==========================================================================
    // Expose this option to UI at some point.
    auto time = juce::Time::getCurrentTime();
    auto logFileName = time.formatted ("Log_%H-%M-%S_%d%b%Y.csv");
    
    auto logFile = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile (logFileName);
    juce::FileOutputStream logStream (logFile);
    logStream.setPosition (0);
    logStream.truncate();
    
    //==========================================================================
    // Write log file
    writeLogHeader (logStream);
    
    logLineCounter = 0;
    
    while (continueLogging)
    {
        logOnsetDetails (logStream);
        std::this_thread::sleep_for (50ms);
    }
}

void EnsembleModel::writeLogHeader (juce::FileOutputStream &logStream)
{
    juce::String logLine ("N");
    juce::String onsetLog, intervalLog, mNoiseLog, tkNoiseLog,
                 asyncLog, alphaLog, tkNoiseStdLog, mNoiseStdLog,
                 velocityLog;
                 
    for (int i = 0; i < players.size(); ++i)
    {
        int playerId = i + 1;
        
        onsetLog += ", P" + juce::String (playerId);
        intervalLog += ", P" + juce::String (playerId) + " Int";
        mNoiseLog += ", P" + juce::String (playerId) + " MVar";
        tkNoiseLog += ", P" + juce::String (playerId) + " TKVar";
        
        for (int j = 0; j < players.size(); ++j)
        {
            int otherPlayerId = j + 1;
            
            asyncLog += ", Async " + juce::String (playerId) + juce::String (otherPlayerId);
            alphaLog += ", Alpha " + juce::String (playerId) + juce::String (otherPlayerId);
        }
        
        tkNoiseStdLog += ", P" + juce::String (playerId) + " TKStd";
        mNoiseStdLog += ", P" + juce::String (playerId) + " MStd";
        velocityLog += ", P" + juce::String (playerId) + " Vol";
    }
    
    logLine += onsetLog + ", " + 
               intervalLog + ", " + 
               mNoiseLog + ", " +
               tkNoiseLog + ", " +
               asyncLog + ", " +
               alphaLog + ", " +
               tkNoiseStdLog + ", " +
               mNoiseStdLog + ", " +
               velocityLog + "\n";
                   
    logStream.writeText (logLine, false, false, nullptr);
}


void EnsembleModel::logOnsetDetails (juce::FileOutputStream &logStream)
{
    while (loggingFifo->getNumReady() > 0)
    {
        std::vector <int> latestOnsets (players.size());
        juce::String logLine (logLineCounter++);
        juce::String onsetLog, intervalLog, mNoiseLog, tkNoiseLog,
                     asyncLog, alphaLog, tkNoiseStdLog, mNoiseStdLog,
                     velocityLog;
        int p = 0;

        auto reader = loggingFifo->read (static_cast <int> (players.size()));
        
        for (int i = 0; i < reader.blockSize1; ++i)
        {
            int onsetTime = onsetBuffer [reader.startIndex1 + i];
            latestOnsets [p++] = onsetTime;
            
            logOnsetDetailsForPlayer (reader.startIndex1 + i,
                                      onsetLog,
                                      intervalLog,
                                      mNoiseLog,
                                      tkNoiseLog,
                                      asyncLog,
                                      alphaLog,
                                      tkNoiseStdLog,
                                      mNoiseStdLog,
                                      velocityLog);                 
        }
        
        for (int i = 0; i < reader.blockSize2; ++i)
        {
            int onsetTime = onsetBuffer [reader.startIndex2 + i];
            latestOnsets [p++] = onsetTime;
     
            logOnsetDetailsForPlayer (reader.startIndex2 + i,
                                      onsetLog,
                                      intervalLog,
                                      mNoiseLog,
                                      tkNoiseLog,
                                      asyncLog,
                                      alphaLog,
                                      tkNoiseStdLog,
                                      mNoiseStdLog,
                                      velocityLog);
        }
        
        logLine += onsetLog + ", " + 
                   intervalLog + ", " + 
                   mNoiseLog + ", " +
                   tkNoiseLog + ", " +
                   asyncLog + ", " +
                   alphaLog + ", " +
                   tkNoiseStdLog + ", " +
                   mNoiseStdLog + ", " +
                   velocityLog + "\n";
                   
        logStream.writeText (logLine, false, false, nullptr);

        // Send onset detail to wherever they need to go.
        postLatestOnsets (latestOnsets);
    }
}

void EnsembleModel::logOnsetDetailsForPlayer (int bufferIndex,
                                              juce::String &onsetLog,
                                              juce::String &intervalLog,
                                              juce::String &mNoiseLog,
                                              juce::String &tkNoiseLog,
                                              juce::String &asyncLog,
                                              juce::String &alphaLog,
                                              juce::String &tkNoiseStdLog,
                                              juce::String &mNoiseStdLog,
                                              juce::String &velocityLog)
{
    onsetLog += ", " + juce::String (onsetBuffer [bufferIndex] / sampleRate);
    intervalLog += ", " + juce::String (onsetIntervalBuffer [bufferIndex] / sampleRate);
    mNoiseLog += ", " + juce::String (motorNoiseBuffer [bufferIndex]);
    tkNoiseLog += ", " + juce::String (timeKeeperNoiseBuffer [bufferIndex]);
    
    for (int i = 0; i < asyncBuffer.size(); ++i)
    {
        asyncLog += "," + juce::String (asyncBuffer [i][bufferIndex] / sampleRate);
        alphaLog += "," + juce::String (alphaBuffer [i][bufferIndex]);
    }
    
    tkNoiseStdLog += ", " + juce::String (tkNoiseStdBuffer [bufferIndex]);
    mNoiseStdLog += ", " + juce::String (mNoiseStdBuffer [bufferIndex]);
    velocityLog += ", " + juce::String (volumeBuffer [bufferIndex]);
}

void EnsembleModel::postLatestOnsets (const std::vector <int> &onsets)
{        
    //==========================================================================
    // onsets contains the onset time in samples for each of the players' most 
    // recently played notes. Do what you will with them here.
        
    // Once you've sent those to the server indicate to the polling thread that
    // it should start to poll for new alphas.
    alphasUpToDate.clear();
}

//==============================================================================
void EnsembleModel::initialisePollingBuffers()
{
    int bufferSize = static_cast <int> (10 * players.size());
    
    pollingFifo = std::make_unique <juce::AbstractFifo> (bufferSize);

    pollingBuffer.resize (players.size());
    
    for (int i = 0; i < pollingBuffer.size(); ++i)
    {
        pollingBuffer [i].resize (bufferSize, 0.0);
    }
}

void EnsembleModel::startPollingLoop()
{
    stopPollingLoop();
    initialisePollingBuffers();
        
    continuePolling = true;
    alphasUpToDate.test_and_set();
    pollingThread = std::thread ([this] () {this->pollingLoop();});
}

void EnsembleModel::stopPollingLoop()
{
    continuePolling = false;
    
    if (pollingThread.joinable())
    {
        pollingThread.join();
    }
}

void EnsembleModel::pollingLoop()
{
    while (continuePolling)
    {
        if (!alphasUpToDate.test_and_set())
        {
            getNewAlphas();
        }
        
        std::this_thread::sleep_for (50ms);
    }
}

void EnsembleModel::getNewAlphas()
{
    //==========================================================================
    // In here you should make a request to your server to ask for new alpha
    // values. If you get some updated values set the following value to true.
    // If not, set the value to false and the plug-in will poll again after
    // short time.
    bool newAlphas = false;
    
    if (newAlphas)
    {
        auto writer = pollingFifo->write (static_cast <int> (players.size()));
        
        for (int player1 = 0; player1 < pollingBuffer.size(); ++player1)
        {        
            int player2 = 0;
            
            for (int i = 0; i < writer.blockSize1; ++i)
            {
                // Replace the 0.2 with the alpha parameter for player1_player2
                pollingBuffer [player1][writer.startIndex1 + i] = 0.2;
                ++player2;
            }
        
            for (int i = 0; i < writer.blockSize2; ++i)
            {
                // Replace the 0.2 with the alpha parameter for player1_player2
                pollingBuffer [player1][writer.startIndex2 + i] = 0.2;
                ++player2;
            }
        }
    }
    else
    {
        alphasUpToDate.clear();
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
