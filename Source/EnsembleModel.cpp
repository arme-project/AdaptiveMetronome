#include "JuceHeader.h"
#include "PluginProcessor.h"
#include "EnsembleModel.h"
#include "UserPlayer.h"

using namespace std::chrono_literals;

//==============================================================================
EnsembleModel::EnsembleModel(AdaptiveMetronomeAudioProcessor* processorPtr)
: processor(processorPtr)
{
    playersInUse.clear();
    resetFlag.clear();

    // OSC Listener addresses
    addListener(this, "/loadConfig");
    addListener(this, "/reset");
    addListener(this, "/setLogname");
    addListener(this, "/numIntroTones");
}

EnsembleModel::~EnsembleModel()
{
    stopLoggerLoop();
    stopPollingLoop();
}

void EnsembleModel::setAlphaBetaParams(float valueIn)
{
    for (int i = 0 ; i < processor->MAX_PLAYERS ; i++)
    {
        for (int j = 0 ; j < processor->MAX_PLAYERS ; j++)
        {
            *processor->alphaParameter(i, j) = valueIn;
        }
    }
}

//==============================================================================
// OSC Messaging
void EnsembleModel::connectOSCSender(int portNumber, juce::String IPAddress = "127.0.0.1")
{
    if (!OSCSender.connect("127.0.0.1", 8000))
        DBG("Error: could not connect to UDP port 8000.");
    else {
        DBG("OSC SENDER CONNECTED");
    }
}

// Connection can be established via config file parameter "OSCReceivePort"
void EnsembleModel::connectOSCReceiver(int portNumber)
{
    if (!connect(portNumber)) 
    {
        currentReceivePort = -1;
        DBG("Error: could not connect to UDP.");
    }
    else
    {
        sendActionMessage("OSC Received");
        currentReceivePort = portNumber;
        DBG("Connection succeeded");
    }
}

bool EnsembleModel::isOscReceiverConnected()
{
    return (currentReceivePort > -1);
}

void EnsembleModel::oscMessageReceived(const juce::OSCMessage& message)
{
    juce::OSCAddressPattern oscPattern = message.getAddressPattern();
    juce::String oscAddress = oscPattern.toString();

    if (oscAddress == "/loadConfig") {
        if (message[0].isString()) {
            auto configFilename = message[0].getString();
            auto configFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(configSubfolder).getChildFile(configFilename);

            if (!configFile.existsAsFile()) { return; }

            loadConfigFromXml(configFile);
        }
    }
    else if (oscAddress == "/reset")
    {
        reset();
    }
    else if (oscAddress == "/setLogname")
    {
        if (message[0].isString())
        {
            auto newLogFilename = message[0].getString();
            if (!newLogFilename.endsWith(".csv")) {
                newLogFilename << ".csv";
            }
            logFilenameOverride = newLogFilename;
        }
    }
    else if (oscAddress == "/numIntroTones")
    {
        if (message[0].isInt32())
        {
            numIntroTones = message[0].getInt32();
        }
    }
    sendActionMessage("OSC Received");
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
// Main method for processing incoming midi stream
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
            playUserIntro (inMidi, outMidi, i);
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

int EnsembleModel::getNumUserPlayers()
{
    return static_cast <int> (numUserPlayers);
}

bool EnsembleModel::isPlayerUserOperated (int playerIndex)
{
    return players [playerIndex]->isUserOperated();
}

juce::AudioParameterInt& EnsembleModel::getPlayerChannelParameter (int playerIndex)
{
    return *processor->channelParameter(playerIndex);
}

juce::AudioParameterFloat& EnsembleModel::getPlayerDelayParameter (int playerIndex)
{
    return *processor->delayParameter(playerIndex);

}

juce::AudioParameterFloat& EnsembleModel::getPlayerMotorNoiseParameter (int playerIndex)
{
    return *processor->mNoiseStdParameter(playerIndex);

}

juce::AudioParameterFloat& EnsembleModel::getPlayerTimeKeeperNoiseParameter (int playerIndex)
{
    return *processor->tkNoiseStdParameter(playerIndex);

}

juce::AudioParameterFloat& EnsembleModel::getPlayerVolumeParameter (int playerIndex)
{
    return *processor->volumeParameter(playerIndex);

}

juce::AudioParameterFloat& EnsembleModel::getAlphaParameter (int player1Index, int player2Index)
{
    return *processor->alphaParameter(player1Index, player2Index);

}

juce::AudioParameterFloat& EnsembleModel::getBetaParameter (int player1Index, int player2Index)
{
    return *processor->betaParameter(player1Index, player2Index);

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
    if (introTonesPlayed % 4 == 0)
    {
        midi.addEvent (juce::MidiMessage::noteOn (introToneChannel, introToneNoteFirst, introToneVel), sampleIndex);
    }
    else
    {
        midi.addEvent (juce::MidiMessage::noteOn (introToneChannel, introToneNoteOther, introToneVel), sampleIndex);
    }
}

void EnsembleModel::introToneOff (juce::MidiBuffer &midi, int sampleIndex)
{
    if (introTonesPlayed % 4 == 0)
    {
        midi.addEvent (juce::MidiMessage::noteOff (introToneChannel, introToneNoteFirst, introToneVel), sampleIndex);
    }
    else
    {
        midi.addEvent (juce::MidiMessage::noteOff (introToneChannel, introToneNoteOther, introToneVel), sampleIndex);
    }}

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
//            players [i]->recalculateOnsetInterval (samplesPerBeat, players, alphaParams [i], betaParams [i]);
            players [i]->recalculateOnsetInterval (samplesPerBeat, players);
        }
    }  
    
    for (int i = 0; i < players.size(); ++i)
    {
        if (players [i]->isUserOperated())
        {
//            players [i]->recalculateOnsetInterval (samplesPerBeat, players, (*alphaParams) [i], (*betaParams) [i]);
            players [i]->recalculateOnsetInterval (samplesPerBeat, players);
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
//    if (pollingFifo)
//    {
//        // Consume everything in the buffer, only using the most recent set of alphas.
//        auto reader = pollingFifo->read (pollingFifo->getNumReady());
//
//        for (int player1 = 0; player1 < pollingBuffer.size(); ++player1)
//        {
//            int player2 = 0;
//
//            int block1Start = std::max (reader.blockSize1 + reader.blockSize2 - static_cast <int> (players.size()), 0);
//
//            for (int i = block1Start; i < reader.blockSize1; ++i)
//            {
//                *(*alphaParams) [player1][player2++] = pollingBuffer [player1][reader.startIndex1 + i];
//            }
//
//            int block2Start = std::max (block1Start - reader.blockSize1, 0);
//
//            for (int i = block2Start; i < reader.blockSize2; ++i)
//            {
//                *(*alphaParams) [player1][player2++] = pollingBuffer [player1][reader.startIndex2 + i];
//            }
//        }
//    }
}

void EnsembleModel::storeOnsetDetailsForPlayer (int bufferIndex, int playerIndex)
{
    // Store the log information about the latest onset from the given player
    // in the logging buffers.
    auto &data = loggingBuffer [bufferIndex];
    
    data.onsetTime = players [playerIndex]->getLatestOnsetTime();
    data.onsetInterval = players [playerIndex]->getPlayedOnsetInterval();
    data.userInput = players [playerIndex]->wasLatestOnsetUserInput();
    data.delay = players [playerIndex]->getLatestOnsetDelay();
    data.motorNoise = players [playerIndex]->getMotorNoise();
    data.timeKeeperNoise = players [playerIndex]->getTimeKeeperNoise();
    
    for (int i = 0; i < players.size(); ++i)
    {
        data.asyncs [i] = players [playerIndex]->getLatestOnsetTime() - players [i]->getLatestOnsetTime();
//        data.alphas [i] = *(*alphaParams) [playerIndex][i];
        data.alphas [i] = processor->alphaParameter(playerIndex , i)->get();

//        data.betas [i] = *(*betaParams) [playerIndex][i];
        data.betas [i] = processor->betaParameter(playerIndex , i)->get();
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
                                                                  samplesPerBeat,
                                                                  processor));
            }
            else
            {
                players.push_back (std::make_unique <Player> (playerIndex++,
                                                              track,
                                                              channelToUse,
                                                              sampleRate,
                                                              scoreCounter,
                                                              samplesPerBeat,
                                                              processor));
            }
        }
    }

    //==========================================================================
    createAlphaBetaParameters(); // create matrix of parameters for alphas
}

// Initialise matrix of alpha and beta parameters
void EnsembleModel::createAlphaBetaParameters()
{
    for (int i = 0; i < players.size(); ++i)
    {
        double alpha = 0.25;
        double beta = 0.1;

        for (int j = 0; j < players.size(); ++j)
        {
            *processor->alphaParameter(i, j) = alpha;
            *processor->betaParameter(i, j) = beta;
        }
    }
}

void EnsembleModel::playUserIntro(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int sampleIndex)
{
    for (auto& player : players)
    {
        if (player->isUserOperated())
        {
            player->processIntroSample(inMidi, outMidi, sampleIndex, introToneNoteOther);
        }
    }
}

// Called from EnsembleModel::processMidiBlock
void EnsembleModel::playScore(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int sampleIndex)
{
    for (auto& player : players)
    {
        player->processSample(inMidi, outMidi, sampleIndex);
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
    introCounter = 0; //-sampleRate / 2;
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
    for (auto& player : players)
    {
        player->reset();
    }

    resetFlag.clear();
}



//==========================================================================
// XML CONFIG FUNCTIONS
// Loading requires converting: xml file -> xmlDocument -> xmlElement

// Converts a .xml file to xmlElement (to be used in loadConfigFromXml)
std::unique_ptr<juce::XmlElement> EnsembleModel::parseXmlConfigFileToXmlElement(juce::File configFile) {
    return juce::XmlDocument(configFile).getDocumentElement();
}

// loadConfigFromXml can be called directly with XmlElement ... or from a File via parseXmlConfigFileToXmlElement
void EnsembleModel::loadConfigFromXml(juce::File configFile) {
    loadConfigFromXml(parseXmlConfigFileToXmlElement(configFile));
}


// Main method to load an XML config file
void EnsembleModel::loadConfigFromXml(std::unique_ptr<juce::XmlElement> loadedConfig)
{
    if (loadedConfig == nullptr) { return; }

    // Flag to keep track if list of players needs to be reinitialised (e.g. number of user players has changed)
    bool playersNeedRecreating = false;
    bool ensembleNeedsResetting = false;

    // "LogSubfolder": Check if new config specifies a new subfolder to save logs to
    if (loadedConfig->hasAttribute("LogSubfolder")) 
    {
        auto newLogSubfolder = loadedConfig->getStringAttribute("LogSubfolder", "");
        if (newLogSubfolder != "")
        {
            logSubfolder = newLogSubfolder;
        }
    }

    // "LogSubfolder": Check if new config specifies a new subfolder to save logs to
    if (loadedConfig->hasAttribute("numIntroTones"))
    {
        numIntroTones = loadedConfig->getIntAttribute("numIntroTones", 7);;
    }
    
    // "ConfigSubfolder": Check if new config specifies new subfolder to look for config and midi files
    if (loadedConfig->hasAttribute("ConfigSubfolder")) 
    {
        auto newConfigSubfolder = loadedConfig->getStringAttribute("ConfigSubfolder", "");
        if (newConfigSubfolder != "")
        {
            configSubfolder = newConfigSubfolder;
        }
    }

    // "LogFilename": Check if log filename should be overriden from default
    if (loadedConfig->hasAttribute("LogFilename")) 
    {
        auto newLogFilename = loadedConfig->getStringAttribute("LogFilename", "");
        if (newLogFilename != "")
        {
            if (!newLogFilename.endsWith(".csv")) {
                newLogFilename << ".csv";
            }
            logFilenameOverride = newLogFilename;
        }
    }

    // "OSCReceivePort":
    // Check if new OSC connections requested
    if (loadedConfig->hasAttribute("OSCReceivePort"))
    {
        auto newOSCReceiverPort = loadedConfig->getIntAttribute("OSCReceivePort");
        if (newOSCReceiverPort != 0)
        {
            connectOSCReceiver(newOSCReceiverPort);
        }
    }


    // "NumUserPlayers": Check if numUserPlayers has changed
    if (loadedConfig->hasAttribute("NumUserPlayers"))
    {
        numUserPlayers = loadedConfig->getIntAttribute("NumUserPlayers");
        playersNeedRecreating = true;
    }
    
    // "MidiFilename": Check if new midi file has been specified in config, and load it.
    if (loadedConfig->hasAttribute("MidiFilename"))
    {
        auto midiFilename = loadedConfig->getStringAttribute("MidiFilename");
        auto midiFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(configSubfolder).getChildFile(midiFilename);
        
        if (!midiFile.existsAsFile()) {return;}

        loadMidiFile(midiFile, numUserPlayers);

        // Players are automatically reinitialised when a new midi file is loaded, so flag can be set back to false
        playersNeedRecreating = false;
    }

    // Limit number of user players to the number of available tracks in the loaded midi file
    if (numUserPlayers > midiFile.getNumTracks()) {
        numUserPlayers = midiFile.getNumTracks();
    }

    if (playersNeedRecreating) {
        createPlayers(midiFile);
        reset();
    }

    // "Alphas" and "Betas": 
    auto xmlAlphas = loadedConfig->getChildByName("Alphas");
    auto xmlBetas = loadedConfig->getChildByName("Betas");

    for (int i = 0; i < players.size(); ++i)
    {
        for (int j = 0; j < players.size(); ++j)
        {
            juce::String xmlAlphaEntryName;
            juce::String xmlBetaEntryName;

            xmlAlphaEntryName << "Alpha_" << i << "_" << j;
            xmlBetaEntryName << "Beta_" << i << "_" << j;

            // If corresponding entries are not found in xml, do not change value
            if (xmlAlphas != nullptr) {
                if (xmlAlphas->hasAttribute(xmlAlphaEntryName)) {
                    *processor->alphaParameter(i,j) = xmlAlphas->getDoubleAttribute(xmlAlphaEntryName);
                }
            }
            if (xmlBetas != nullptr) {
                if (xmlBetas->hasAttribute(xmlBetaEntryName)) {
                    *processor->betaParameter(i,j) = xmlBetas->getDoubleAttribute(xmlBetaEntryName);

                }
            }
        }
    }

    // "Motor" and "Timekeeper" noise:
    auto xmlTkNoise = loadedConfig->getChildByName("tkNoise");
    auto xmlMNoise = loadedConfig->getChildByName("mNoise");

    for (int i = 0; i < players.size(); ++i)
    {
        juce::String xmlTkNoiseEntryName;
        juce::String xmlMNoiseEntryName;

        xmlTkNoiseEntryName << "tkNoise_" << i;
        xmlMNoiseEntryName << "mNoise_" << i;

        // If corresponding entries are not found in xml, do not change value
        if (xmlTkNoise != nullptr) {
            if (xmlTkNoise->hasAttribute(xmlTkNoiseEntryName)) {
                *processor->tkNoiseStdParameter(i) = xmlTkNoise->getDoubleAttribute(xmlTkNoiseEntryName);

            }
        }
        if (xmlMNoise != nullptr) {
            if (xmlMNoise->hasAttribute(xmlMNoiseEntryName)) {
                *processor->mNoiseStdParameter(i) = xmlMNoise->getDoubleAttribute(xmlTkNoiseEntryName);
            }
        }
    }

    sendActionMessage("Ensemble Reset");

    if (ensembleNeedsResetting) {
        reset();
    }
}

// Formats the current ensemble state to xml, and saves it to a file (currently a default file in user folder)
// Note: This currently only saves alpha and beta parameters. 
void EnsembleModel::saveConfigToXmlFile()
{
    #ifdef JUCE_WINDOWS
    auto xmlOutput = &juce::XmlElement("EnsembleModelConfig");
    xmlOutput->setAttribute("numUserPlayers", numUserPlayers);

    auto xmlAlphas = xmlOutput->createNewChildElement("Alphas");
    auto xmlBetas = xmlOutput->createNewChildElement("Betas");
    for (int i = 0; i < players.size(); ++i)
    {
        for (int j = 0; j < players.size(); ++j)
        {
            float alpha = getAlphaParameter(i, j);
            float beta = getBetaParameter(i, j);

            juce::String xmlAlphaEntryName;
            juce::String xmlBetaEntryName;

            xmlAlphaEntryName << "Alpha_" << i << "_" << j;
            xmlBetaEntryName << "Beta_" << i << "_" << j;

            xmlAlphas->setAttribute(xmlAlphaEntryName, alpha);
            xmlBetas->setAttribute(xmlBetaEntryName, beta);
        }
    }
    
    auto ensembleConfigFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("EnsembleModelConfig.xml");
    xmlOutput->writeTo(ensembleConfigFile);
    #endif
}

//==============================================================================
// Logging functionality
void EnsembleModel::initialiseLoggingBuffer()
{
    int bufferSize = 0;
    if (players.size() > 0)
    {
        bufferSize = static_cast <int> (4 * players.size());
    }
    else {
        bufferSize = 4;
    }
    
    loggingFifo = std::make_unique <juce::AbstractFifo> (bufferSize);
    
    loggingBuffer.resize (bufferSize);
    
    for (int i = 0; i < loggingBuffer.size(); ++i)
    {
        loggingBuffer [i].asyncs.resize (players.size(), 0.0);
        loggingBuffer [i].alphas.resize (players.size(), 0.0);
        loggingBuffer [i].betas.resize (players.size(), 0.0);
    }
}

void EnsembleModel::startLoggerLoop()
{
    stopLoggerLoop();
    initialiseLoggingBuffer();
        
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

    // Start with default documents folder
    juce::File logFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);

    // Add subfolder, if this is specified
    if (logSubfolder != "")
    {
        logFile = logFile.getChildFile (logSubfolder);
        if (!logFile.exists()) {
            logFile.createDirectory();
        }
    }

    // Check if the log filename has also been overriden via config. 
    if (logFilenameOverride != "") {
        // TODO What happens if overriden log file already exists? Override? Create a new one with slightly different name? 
        logFile = logFile.getChildFile(logFilenameOverride).getNonexistentSibling();
    }
    else {
        auto logFileName = time.formatted("Log_%H-%M-%S_%d%b%Y.csv");
        logFile = logFile.getChildFile(logFileName);
    }

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
    juce::String onsetLog, intervalLog, userInputLog, delayLog,
                 mNoiseLog, tkNoiseLog, asyncLog, alphaLog, betaLog,
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
            betaLog += ", Beta " + juce::String (playerId) + juce::String (otherPlayerId);
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
               betaLog + ", " +
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
                     mNoiseLog, tkNoiseLog, asyncLog, alphaLog, betaLog,
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
                                      betaLog,
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
                                      betaLog,
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
                   betaLog + ", " +
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
                                              juce::String &betaLog,
                                              juce::String &tkNoiseStdLog,
                                              juce::String &mNoiseStdLog,
                                              juce::String &velocityLog)
{
    auto &data = loggingBuffer [bufferIndex];
    
    onsetLog += ", " + juce::String (data.onsetTime / sampleRate);
    intervalLog += ", " + juce::String (data.onsetInterval / sampleRate);
    userInputLog += ", " + juce::String (data.userInput ? "true" : "false");
    delayLog += ", " + juce::String (data.delay / sampleRate);
    mNoiseLog += ", " + juce::String (data.motorNoise);
    tkNoiseLog += ", " + juce::String (data.timeKeeperNoise);
    
    for (int i = 0; i < data.asyncs.size(); ++i)
    {
        asyncLog += "," + juce::String (data.asyncs [i] / sampleRate);
        alphaLog += "," + juce::String (data.alphas [i]);
        betaLog += "," + juce::String (data.betas [i]);
    }
    
    tkNoiseStdLog += ", " + juce::String (data.tkNoiseStd);
    mNoiseStdLog += ", " + juce::String (data.mNoiseStd);
    velocityLog += ", " + juce::String (data.volume);
}

// NOT IMPLEMENTED
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

// NOT USED
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
