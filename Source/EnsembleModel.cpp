#include "EnsembleModel.h"
#include "UserPlayer.h"
#include "MatlabEngine.hpp"
#include "MatlabDataArray.hpp"
#include "recalculateAlphas.h"
#include "getAlphas_terminate.h"

using namespace matlab::engine;
using namespace matlab::data ;
using namespace std::chrono;

//==============================================================================
EnsembleModel::EnsembleModel()
{   
    randomizer = juce::Random(0);
    playersInUse.clear();
    resetFlag.clear();
    currentNoteIndex.set(99);
    // specify here where to send OSC messages to: host URL and UDP port number
    if (!sender.connect("127.0.0.1", 8000))
        DBG("Error: could not connect to UDP port 8000.");
    else {
        DBG("OSC SENDER CONNECTED");
    }
    auto sharedMATLABs = matlab::engine::findMATLAB();
    if (sharedMATLABs.size() > 0) {
        //matlabEngine = connectMATLAB(u"MATLAB_4016");
        matlabEngine = connectMATLAB(sharedMATLABs[0]);
        //matlabEngine->eval(u"[X, Y] = meshgrid(-2:0.2:2);");
    }

    auto alpha1 = getAlphasCppTest();
    DBG(alpha1);
    getAlphas_terminate();

}

EnsembleModel::~EnsembleModel()
{
    stopLoggerLoop();
    stopPollingLoop();
}

void EnsembleModel::writeToLogger(time_point<system_clock> timeStamp, juce::String source, juce::String method, juce::String message) {
    auto timeStampString = juce::String(clock->tickToString(timeStamp));
    juce::String formattedString;
    formattedString << timeStampString
        << " - "
        << source << "::" << method
        << " --- "
        << message;
    juce::Logger::writeToLog(formattedString);
}

bool EnsembleModel::getAlphasFromMATLAB(bool test = false) {
    // This is the main method to call to update alphas
    // This packs the most recent onsets, sends to Matlab,
    // and assigns returned values to alphaParams
    
    // If connected to matlab instance ...
    if (matlabEngine == nullptr) {
        DBG("NO MATLAB INSTANCE CONNECTED");
        return false;
    }

    std::vector<matlab::data::Array> matlabOnsetArray = buildMatlabOnsetArray(test);
        
    if (logToLogger) {
        juce::String message;
        message << "Array sizes before sending to MATLAB: ";
        for (matlab::data::Array array : matlabOnsetArray) {
        //for (auto array : matlabOnsetArray) {
            message << array.getNumberOfElements() << ", ";
        }
        writeToLogger(clock->tick(), "EnsembleModel",
            "getAlphasFromMATLAB", message);
    }

    TypedArray<double> alphasFromMatlab = factory.createEmptyArray();
    bool useFEVAL = false;
    if (useFEVAL) {
        alphasFromMatlab = 
            matlabEngine->feval(u"getAlphas", matlabOnsetArray);
    }
    else {
        for (int nPlayer = 0; nPlayer < 4; nPlayer++) {
            TypedArray<double> matlabArray = matlabOnsetArray[nPlayer];
            juce::String varName;
            varName << "P" << nPlayer;

            matlabEngine->setVariable(varName.toStdString(), matlabArray);
        }
        matlabEngine->eval(u"alphas=getAlphas(P0, P1, P2, P3, true)");
        alphasFromMatlab = matlabEngine->getVariable(u"alphas");
    }

    if (logToLogger) {
        juce::String message;
        message << "Returned alphas from MATLAB: ";
        for (auto size : alphasFromMatlab.getDimensions()) {
            message << size << ", ";
        }
        writeToLogger(clock->tick(), "EnsembleModel",
            "getAlphasFromMATLAB", message);
    }

    if (setAlphasFromMATLABArray(alphasFromMatlab)) {
        return true;
    }
    else {
        return false;
    }
}

bool EnsembleModel::getAlphasFromCodegen(bool test = false) {
    // This is the main method to call to update alphas
    // This packs the most recent onsets, sends to Matlab,
    // and assigns returned values to alphaParams

    // If connected to matlab instance ...

    auto alphasFromCodegen = getAlphasCpp(players[0]->onsetTimes, players[1]->onsetTimes, players[2]->onsetTimes, players[3]->onsetTimes);

    if (setAlphasFromCodegen(alphasFromCodegen)) {
        return true;
    }
    else {
        return false;
    }
}

bool EnsembleModel::setAlphasFromCodegen(std::vector<std::vector<double>> alphasFromCodegen) {

    for (int n = 0; n < 4; n++) {
        for (int m = 0; m < 4; m++) {
            *alphaParams[n][m] = alphasFromCodegen[n][m];
        }
    }

    return true;
}


bool EnsembleModel::setAlphasFromMATLABArray(TypedArray<double> alphasFromMATLAB)
{
    if (alphasFromMATLAB.getNumberOfElements() == 16) {
        int playerCount = 0;
        int valueCount = 0;
        int totalCount = 0;
        for (auto dim : alphasFromMATLAB) {
            valueCount = (int)(totalCount / 4);
            playerCount = (int)(totalCount % 4);



            if (getNumPlayers() > 0) {
                *alphaParams[playerCount][valueCount] = (float)dim;
            }
            totalCount++;
        }
        return true;
    }
    else {
        DBG("WRONG NUMBER OF ELEMENTS IN MATLAB RETURN ARRAY");
        return false;
    }
}

std::vector<matlab::data::Array> EnsembleModel::buildMatlabOnsetArray(bool test = false) {
    // This packs the recent onsets as a Matlab array to be sent to Matlab
    // if test == true, 20 onsets are randomly generated
    
    // Vector to store Matlab arrays of function inputs
    if (getNumPlayers() != 4) {
        DBG("NUMBER OF PLAYERS MUST BE 4 FOR ALPHA CALC");
        DBG("Using test numbers for calc");
        test = true;
    }
    auto vectorOfOnsetArrays = std::vector<Array>(4);

    if (!test) {
        for (int nPlayer = 0; nPlayer < getNumPlayers(); nPlayer++) {
            //auto intervals = &players[nPlayer]->onsetIntervals;
            auto intervals = &players[nPlayer]->onsetTimes;

            TypedArray<double> data = factory.createArray<std::deque<double>::iterator>(
                { intervals->size(), 1 },
                intervals->begin(), intervals->end());
            vectorOfOnsetArrays[nPlayer] = data;
        }
    }
    else {

        for (int nPlayer = 0; nPlayer < 4; nPlayer++) {
            // Generate random intervals for testing
            std::deque<double> onsetIntervals;
            auto testInterval = double(0.5);
            for (int i = 0; i < 20; i++) {
                auto randInterval = 0.5 + (randomizer.nextFloat() / 20);
                onsetIntervals.push_back(randInterval);
            }

            // Convert to Matlab array
            TypedArray<double> data = factory.createArray<std::deque<double>::iterator>({ onsetIntervals.size(), 1 },
                onsetIntervals.begin(), onsetIntervals.end());

            // Add to vector
            vectorOfOnsetArrays[nPlayer] = data;
        }
    }
    return vectorOfOnsetArrays;
}

void EnsembleModel::oscMessageSend(bool test)
{
    if (test) {
        auto oscMessage = juce::OSCMessage("/test");
        if (!sender.send(oscMessage)) {
            DBG("Error: could not send OSC message.");
        }
    }
    else {

        auto oscMessage = juce::OSCMessage("/onsets");
        for (int i = 0; i < 4; i++) {
            auto randomFloat = randomizer.nextFloat()/(float)20.0 + (float)0.5;
            oscMessage.addArgument(randomFloat);
        }

        if (!sender.send(oscMessage)) {
            DBG("Error: could not send OSC message.");
        }
    }
}
//==============================================================================
bool EnsembleModel::loadMidiFile (const juce::File &file, int userPlayers)
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
    numUserPlayers = userPlayers;
    createPlayers (midiFile); // create new players
    resetPlayers();
    
    return true;
}

void EnsembleModel::triggerFirstNote() {
    waitingForFirstNote = false;
}

bool EnsembleModel::reset()
{
    FlagLock lock (playersInUse);
    
    if (!lock.locked)
    {
        return false;
    }
    
    resetPlayers();
    playbackStarted = false;
    
    waitingForFirstNote = true;
    return true;
}

bool EnsembleModel::reset(bool skipIntroNotes)
{
    reset();
    introTonesPlayed = numIntroTones;

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
    //DBG(numSamples);
    FlagLock lock (playersInUse);
    
    if (!lock.locked || waitingForFirstNote)
    {
        return;
    }
    
    //if (!firstSampleProcessed) {
    //    DBG(clock->tickToString(clock->tick()));
    //    clock->setStartOfFirstSample(clock->tick());
    //    firstSampleProcessed = true;
    //}

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
        else {
            if (!playbackStarted) {
                //oscMessageSend();
            }
        }

        playScore(inMidi, outMidi, i);
    }
    
}

void EnsembleModel::setUserOnsetFromOsc(float oscOnsetTime, int onsetNoteNumber, int msMax)
{
    for (auto& player : players)
    {
        if (player->isUserOperated()) {
            auto tickOfOnset = clock->convertMsMaxToTick(msMax);
            int msOnsetSinceFirstSample = clock->getDurationSincePlayback(tickOfOnset);
            int onsetInSamples = (int)(msOnsetSinceFirstSample * (sampleRate / 1000));
            player->setOscOnsetTime(oscOnsetTime, onsetNoteNumber, onsetInSamples);
        }
    }
}

void EnsembleModel::playScore(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int sampleIndex)
{
    if (!firstSampleProcessed) {
        DBG("Since start = " << clock->getDurationSincePlayback(clock->tick()));
        clock->setStartOfFirstSample(clock->tick());
        firstSampleProcessed = true;
    }
    //if (scoreCounter % (int)sampleRate == 0) {
        //DBG("Since start = " << clock->getDurationSincePlayback(clock->tick()));
        //DBG(clock->tickToString(clock->tick()));
    //}
    for (auto& player : players)
    {
        player->processSample(inMidi, outMidi, sampleIndex);
        if (player->isUserOperated()) {
            if (player->userPlayedNote) {
                //DBG("Player played note at " << player->getLatestOnsetTimeInSamples() / sampleRate);
                player->userPlayedNote = false;
            }
        }
    }
    //if (players[1]->hasNotePlayed()) {
    //    DBG("Next note for user is " << players[1]->getCurrentNoteIndex() + 1 << " at time " << (players[1]->getLatestOnsetTimeInSamples() + players[1]->getOnsetIntervalForNextNote())/sampleRate);
    //}

    // If all players have played a note, update timings.
    //if (newOnsetsAvailable())
    //{
    //    calculateNewIntervals();
    //    clearOnsetsAvailable();
    //}
    
    ++scoreCounter;
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

juce::AudioParameterFloat& EnsembleModel::getPlayerDelayParameter (int playerIndex)
{
    return players [playerIndex]->delayParam;
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

void EnsembleModel::setMetronomeClock(MetronomeClock* clockPtr)
{
    clock = clockPtr;
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
    //DBG("Setting new tempo to: " << bpm << ":" << samplesPerBeat);
    
    setInitialPlayerTempo();
}

void EnsembleModel::setInitialPlayerTempo()
{
    if (!initialTempoSet)
    {
        for (auto &player : players)
        {
            player->setOnsetIntervalForNextNote (samplesPerBeat);
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
    auto minimumNumberOfOnsets = 5;
    bool enoughOnsetsToRecalculateAlpha = true;
    for (int i = 0; i < getNumPlayers(); ++i) {
        if (players[i]->onsetIntervals.size() < minimumNumberOfOnsets) {
            enoughOnsetsToRecalculateAlpha = false;
        }
    }
    //if (enoughOnsetsToRecalculateAlpha) {
    //    //if (!getAlphasFromMATLAB()) {
    //    //    DBG("ALPHAS CANT BE UPDATED");
    //    //}
    //    if (!getAlphasFromCodegen()) {
    //        DBG("ALPHAS CANT BE UPDATED");
    //    }
    //}
    //else {
    //    DBG("NOT ENOUGH ONSETS TO RECALCULATE ALPHA");
    //}
    //==========================================================================
    // Calculate new onset times for players.
    // Make sure all non-user players update before the user players.
    for (int i = 0; i < players.size(); ++i)
    {
        if (!players [i]->isUserOperated())
        {
            players [i]->recalculateOnsetInterval (samplesPerBeat, players, alphaParamsFixed [i]);
        }
        if (i == 3) {
            currentNoteIndex.set(players[i]->getCurrentNoteIndex());
        }
    }  
    
    for (int i = 0; i < players.size(); ++i)
    {
        if (players [i]->isUserOperated())
        {
            players [i]->recalculateOnsetInterval (samplesPerBeat, players, alphaParamsFixed [i]);
        }
    } 
         
    for (int i = 0; i < players.size(); ++i)
    {
        oscMessageSendNewInterval(i, players[i]->getCurrentNoteIndex() + 1, players[i]->getNextNoteTimeInMS());
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

//==============================================================================

void EnsembleModel::oscMessageSendNewInterval(int playerNum, int noteNum, int noteTimeInMS) {

    auto oscMessage = juce::OSCMessage("/newInterval");
    oscMessage.addInt32(playerNum);
    oscMessage.addInt32(noteNum);
    oscMessage.addInt32(noteTimeInMS);
    if (!sender.send(oscMessage)) {
        DBG("Error: could not send OSC message.");
    }

}

void EnsembleModel::oscMessageSendReset() {

    auto oscMessage = juce::OSCMessage("/reset");
    if (!sender.send(oscMessage)) {
        DBG("Error: could not send OSC message.");
    }
}

void EnsembleModel::oscMessageSendPlayMax() {
    auto oscMessage = juce::OSCMessage("/playMax");
    if (!sender.send(oscMessage)) {
        DBG("Error: could not send OSC message.");
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
    // Store the log information about the latest onset from the given player
    // in the logging buffers.
    auto &data = loggingBuffer [bufferIndex];
    
    data.onsetTime = players [playerIndex]->getLatestOnsetTimeInSamples();
    data.onsetIntervalForNextNote = players [playerIndex]->getPlayedOnsetIntervalInSamples();
    data.userInput = players [playerIndex]->wasLatestOnsetUserInput();
    data.delay = players [playerIndex]->getLatestOnsetDelay();
    data.motorNoise = players [playerIndex]->getMotorNoise();
    data.timeKeeperNoise = players [playerIndex]->getTimeKeeperNoise();
    
    for (int i = 0; i < players.size(); ++i)
    {
        data.asyncs [i] = players [playerIndex]->getLatestOnsetTimeInSamples() - players [i]->getLatestOnsetTimeInSamples();
        data.alphas [i] = *alphaParams [playerIndex][i];
    }
    
    data.tkNoiseStd = players [playerIndex]->getTimeKeeperNoiseStd();
    data.mNoiseStd = players [playerIndex]->getMotorNoiseStd();
    data.volume = players [playerIndex]->getLatestVolume();
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
            
            if (playerIndex < numUserPlayers)
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
    alphaParamsFixed.clear();
    for (int i = 0; i < players.size(); ++i)
    {
        std::vector <std::unique_ptr <juce::AudioParameterFloat> > row;
        std::vector <std::unique_ptr <juce::AudioParameterFloat> > rowFixed;

        // CHANGE DEFAULT ALPHA
        double alpha = 0.25;

        for (int j = 0; j < players.size(); ++j)
        {
            row.push_back(std::make_unique <juce::AudioParameterFloat>("alpha-" + juce::String(i) + "-" + juce::String(j),
                "Alpha " + juce::String(i) + "-" + juce::String(j),
                -1.0, 1.0, alpha));

            rowFixed.push_back(std::make_unique <juce::AudioParameterFloat>("alpha-" + juce::String(i) + "-" + juce::String(j),
                "Alpha " + juce::String(i) + "-" + juce::String(j),
                -1.0, 1.0, alpha));

            alpha = 0.0;
        }

        alphaParams.push_back(std::move(row));
        alphaParamsFixed.push_back(std::move(rowFixed));
    }
}

void EnsembleModel::resetPlayers()
{
    //==========================================================================
    // Initialise intro countdown
    introCounter = -sampleRate / 2;
    introTonesPlayed = 0;
     
    // Initialise score counter
    scoreCounter = 0;
    firstSampleProcessed = false;
    
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
void EnsembleModel::initialiseLoggingBuffer()
{
    if (players.size() > 0) {
        int bufferSize = static_cast <int> (4 * players.size());

        loggingFifo = std::make_unique <juce::AbstractFifo>(bufferSize);

        loggingBuffer.resize(bufferSize);

        for (int i = 0; i < loggingBuffer.size(); ++i)
        {
            loggingBuffer[i].asyncs.resize(players.size(), 0.0);
            loggingBuffer[i].alphas.resize(players.size(), 0.0);
        }
    }
}

void EnsembleModel::startLoggerLoop()
{
    stopLoggerLoop();
    initialiseLoggingBuffer();
        
    continueLogging = true;
    loggerThread = std::thread ([this] () {this->loggerLoop();});
}

void EnsembleModel::startPollingLoop()
{
    stopPollingLoop();

    continuePolling = true;
    pollingThread = std::thread([this]() {this->pollingLoop(); });
}

void EnsembleModel::stopLoggerLoop()
{
    continueLogging = false;
    
    if (loggerThread.joinable())
    {
        loggerThread.join();
    }
}

void EnsembleModel::stopPollingLoop()
{
    continuePolling = false;

    if (pollingThread.joinable())
    {
        pollingThread.join();
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

void EnsembleModel::pollingLoop()
{
    while (continuePolling) {
        if (newOnsetsAvailable())
            {
                calculateNewIntervals();
                clearOnsetsAvailable();
            }
        std::this_thread::sleep_for(10ms);
    }
}

void EnsembleModel::writeLogHeader (juce::FileOutputStream &logStream)
{
    juce::String logLine ("N");
    juce::String onsetLog, intervalLog, userInputLog, delayLog,
                 mNoiseLog, tkNoiseLog, asyncLog, alphaLog, 
                 tkNoiseStdLog, mNoiseStdLog, velocityLog;
                 
    for (int i = 0; i < players.size(); ++i)
    {
        int playerId = i + 1;
        
        onsetLog += ", P" + juce::String (playerId) + (players [i]->isUserOperated() ? " (input)" : "");
        intervalLog += ", P" + juce::String (playerId) + " Int";
        userInputLog += ", P" + juce::String (playerId) + " User Input";
        delayLog += ", P" + juce::String (playerId) + " Delay";
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
               userInputLog + ", " + 
               delayLog + ", " +
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
        std::vector <int> latestOnsets (players.size()), latestDelays (players.size());
        juce::String logLine (logLineCounter++);
        juce::String onsetLog, intervalLog, userInputLog, delayLog,
                     mNoiseLog, tkNoiseLog, asyncLog, alphaLog, 
                     tkNoiseStdLog, mNoiseStdLog, velocityLog;
                 
        int p = 0;

        auto reader = loggingFifo->read (static_cast <int> (players.size()));
        
        for (int i = 0; i < reader.blockSize1; ++i)
        {
            // Append to array to send to server.
            int bufferIndex = reader.startIndex1 + i;
            
            auto &data = loggingBuffer [bufferIndex];
            latestOnsets [p] = data.onsetTime;
            latestDelays [p] = data.delay;
            ++p;
            
            // Log to log file
            logOnsetDetailsForPlayer (bufferIndex,
                                      onsetLog,
                                      intervalLog,
                                      userInputLog,
                                      delayLog,
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
             // Append to array to send to server.
            int bufferIndex = reader.startIndex2 + i;
            
            auto &data = loggingBuffer [bufferIndex];
            latestOnsets [p] = data.onsetTime;
            latestDelays [p] = data.delay;
            ++p;
     
            // Log to log file
            logOnsetDetailsForPlayer (bufferIndex,
                                      onsetLog,
                                      intervalLog,
                                      userInputLog,
                                      delayLog,
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
                   userInputLog + ", " + 
                   delayLog + ", " +
                   mNoiseLog + ", " +
                   tkNoiseLog + ", " +
                   asyncLog + ", " +
                   alphaLog + ", " +
                   tkNoiseStdLog + ", " +
                   mNoiseStdLog + ", " +
                   velocityLog + "\n";
                   
        logStream.writeText (logLine, false, false, nullptr);

        // Send onset detail to wherever they need to go.
        postLatestOnsets (latestOnsets, latestDelays);
    }
}

void EnsembleModel::logOnsetDetailsForPlayer (int bufferIndex,
                                              juce::String &onsetLog,
                                              juce::String &intervalLog,
                                              juce::String &userInputLog,
                                              juce::String &delayLog,
                                              juce::String &mNoiseLog,
                                              juce::String &tkNoiseLog,
                                              juce::String &asyncLog,
                                              juce::String &alphaLog,
                                              juce::String &tkNoiseStdLog,
                                              juce::String &mNoiseStdLog,
                                              juce::String &velocityLog)
{
    auto &data = loggingBuffer [bufferIndex];
    
    onsetLog += ", " + juce::String (data.onsetTime / sampleRate);
    intervalLog += ", " + juce::String (data.onsetIntervalForNextNote / sampleRate);
    userInputLog += ", " + juce::String (data.userInput ? "true" : "false");
    delayLog += ", " + juce::String (data.delay / sampleRate);
    mNoiseLog += ", " + juce::String (data.motorNoise);
    tkNoiseLog += ", " + juce::String (data.timeKeeperNoise);
    
    for (int i = 0; i < data.asyncs.size(); ++i)
    {
        asyncLog += "," + juce::String (data.asyncs [i] / sampleRate);
        alphaLog += "," + juce::String (data.alphas [i]);
    }
    
    tkNoiseStdLog += ", " + juce::String (data.tkNoiseStd);
    mNoiseStdLog += ", " + juce::String (data.mNoiseStd);
    velocityLog += ", " + juce::String (data.volume);
}

void EnsembleModel::postLatestOnsets (const std::vector <int> &onsets, const std::vector <int> &delays)
{        
    //==========================================================================
    // onsets contains the onset time in samples for each of the players' most 
    // recently played notes.
    //
    // delays contains the delays for each player in samples
    //
    // Send these to the server however you want here. The current sampling
    // frequency is available in the sampleRate variable.
        
    // Once you've sent those to the server indicate to the polling thread that
    // it should start to poll for new alphas.
    alphasUpToDate.clear();
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
