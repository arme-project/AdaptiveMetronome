/**
 * \file EnsembleModel.h
 * \brief Header file for the EnsembleModel class.
 *
 * This file contains the definition of the EnsembleModel class, which is responsible for managing the ensemble of players, their states, and communication with the Max/MSP patch.
 * It handles loading MIDI files, processing MIDI data, and sending OSC messages to Max/MSP.
 */

#pragma once
#include <JuceHeader.h>
#include <vector>
#include <atomic>
#include <thread>
#include "Logger.h"
#include "Player.h"

using std::function;

class AdaptiveMetronomeAudioProcessor;

using AudioParameterFloatToUse = juce::AudioParameterFloat;

class EnsembleModel : private juce::OSCReceiver,
                      private juce::OSCReceiver::ListenerWithOSCAddress<juce::OSCReceiver::MessageLoopCallback>,
                      public juce::ActionBroadcaster
{
public:
    //==============================================================================
    // EnsembleModel();

    /**
     * \brief Constructor for EnsembleModel.
     *
     * \param processorPtr The audio processor which owns this model.
     */
    EnsembleModel(AdaptiveMetronomeAudioProcessor *processorPtr);

    /**
     * \brief Destructor for the EnsembleModel class.
     * Stops the logger loop and polling loop to ensure proper cleanup of resources.
     */
    ~EnsembleModel();

    AdaptiveMetronomeAudioProcessor *processor = nullptr;
    //==============================================================================
    // Communication
    
    bool OSCAutoConnect = false;

    /**
     * \brief Sends an OSC message either as a test or with onset data.
     * 
     * \param test If true, sends a test OSC message to the "/test" address.
     *             Otherwise, sends an OSC message to the "/onsets" address
     *             with four random float arguments.
     */
    void oscMessageSend(bool test);

    /**
     * Sends an OSC message to notify Max of a new interval. The message contains three integers: the player number, the note number, and the note time in milliseconds.
     * \param playerNum The player number.
     * \param noteNum The note number.
     * \param noteTimeInMS The note time in milliseconds.
     */
    void oscMessageSendNewInterval(int playerNum, int noteNum, int noteTimeInMS);
    
    /**
     * \brief Sends an OSC message to trigger a reset in the Max/MSP patch.
     */
    void oscMessageSendReset();

    /**
     * \brief Send an OSC message to trigger Max to start playing.
     *
     * The message is /playMax. This is used to trigger Max to start playing
     * after the user has pressed play in the plugin editor.
     */
    void oscMessageSendPlayMax();



    //==============================================================================
    
    /**
     *  \brief Loads a MIDI file into the ensemble model.
     *
     *  The file is read into a juce::MidiFile, and then the tracks are converted into
     *  Player objects. The userPlayers parameter determines how many of the tracks
     *  are set to be user-operated. The method returns false if the file cannot be
     *  read, or if the playersInUse flag is set.

     *   \param file The MIDI file to load.
     *   \param userPlayers The number of user players tracks in the file.
     *   \return Whether the file was loaded successfully.
     */
    bool loadMidiFile(const juce::File &file, int userPlayers);
    
    /**
     * \brief Resets all players to the beginning of their scores, and resets the onset times of all players to 0.0.
     * \return Returns false if the players are currently in use.
     */
    bool reset();
    
    /**
     * Resets the model to its initial state, and setting the number of Intro Tones.
     *
     * @param skipIntroNotes UNUSED
     *
     * @returns false always.
     */
    bool reset(bool skipIntroNotes);
    
    /**
     * \brief Sets the tempo of the ensemble to the given beats per minute (bpm).
     *
     * If the tempo has not actually changed, the function returns without doing anything.
     *
     * Otherwise, it updates the samplesPerBeat variable of the ensemble, and calls the
     * setInitialPlayerTempo() function to set the initial onset interval of each player to
     * the new tempo.
     *
     * \param bpm the tempo in beats per minute
     */
    void setTempo(double bpm);

    //==============================================================================
    /**
     * \brief Sets the sample rate of the ensemble model.
     * 
     * \param newSampleRate The new sample rate to set.
     */
    void prepareToPlay(double newSampleRate);

    /**
     * \brief Releases any resources used by the ensemble model.
     * 
     * This function is called when the ensemble model is no longer needed. This function is
     * currently not implemented.
     */
    void releaseResources();

    //==============================================================================
    
    /**
     * \brief Processes a block of incoming MIDI data and generates corresponding output MIDI data.
     *
     * This function updates the tempo based on the DAW playhead and processes each sample
     * of the MIDI buffer. If the ensemble has been reset, it clears the output MIDI buffer.
     * It handles the playback of introductory tones and user intros before proceeding to
     * play the main score for each sample in the buffer.
     *
     * \param inMidi The incoming MIDI data buffer.
     * \param outMidi The output MIDI data buffer.
     * \param numSamples The number of samples in the MIDI buffer.
     * \param tempo The current tempo in beats per minute.
     */
    void processMidiBlock(const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int numSamples, double tempo);
    
    /**
     * \brief This synchronises both Max MSP and the Adapative Metronone.
     * 
     * This method is called from the PluginProcessor when a note is played via OSC from Max
     * oscOnsetTime is in seconds, msMax is the time of the onset in ms according to Max's cpu clock.
     * 
     * \param oscOnsetTime The onset time from the OSC message, in seconds.
     * \param onsetNoteNumber The note number of the note that was played.
     * \param msMax The time of the onset in ms according to Max's cpu clock.
     */
    void setUserOnsetFromOsc(float oscOnsetTime, int onsetNoteNumber, int msMax);

    //==============================================================================
    bool waitingForFirstNote = true;

    /**
     * \brief Trigger the first note to be played by the player.
     *
     * \details Sets waitingForFirstNote to false.
     */
    void triggerFirstNote();

    /**
     * \brief Gets the number of players in the ensemble.
     */
    int getNumPlayers();
    
    /**
     * \brief Gets the number of user players in the ensemble.
     */
    int getNumUserPlayers();
    
    /**
     * \brief Checks if a player is user operated or not.
     */
    bool isPlayerUserOperated(int playerIndex);
    
    
    // Getters for player parameters
    juce::AudioParameterInt &getPlayerChannelParameter(int playerIndex);
    AudioParameterFloatToUse &getPlayerDelayParameter(int playerIndex);
    AudioParameterFloatToUse &getPlayerMotorNoiseParameter(int playerIndex);
    AudioParameterFloatToUse &getPlayerTimeKeeperNoiseParameter(int playerIndex);
    AudioParameterFloatToUse &getPlayerVolumeParameter(int playerIndex);
    AudioParameterFloatToUse &getAlphaParameter(int player1Index, int player2Index);
    AudioParameterFloatToUse &getBetaParameter(int player1Index, int player2Index);

    // TODO: Change to a JUCE parameter
    juce::Atomic<int> currentNoteIndex;
    //==============================================================================
    
    /**
     * \brief Sends messages to turn off all notes, sounds and controllers on all
     * MIDI channels to the given MidiBuffer. This is useful for stopping
     * all sound when the user closes the plugin or changes presets.
     * 
     * \param midi The MidiBuffer to send messages to.
     */
    static void soundOffAllChannels(juce::MidiBuffer &midi);

    // Functions for storing and loading ensemble config from XML file

    /**
    * \brief Saves the current configuration of the ensemble model to an XML file.
    */
    void saveConfigToXmlFile();
    std::unique_ptr<juce::XmlElement> parseXmlConfigFileToXmlElement(juce::File configFile);

    /**
    * \brief Saves the current configuration of the ensemble model to an XML file.
    */
    void loadConfigFromXml(std::unique_ptr<juce::XmlElement> loadedConfig);
    
    /**
     * Converts a given XML file into an XmlElement.
     * 
     * \param configFile The juce::File representing the XML file to be parsed.
     * \return A unique_ptr to a juce::XmlElement that represents the root element of the parsed XML document.
     */
    void loadConfigFromXml(juce::File configFile);

    // OSC Messaging
    juce::OSCSender OSCSender;

    /**
     * Connect the OSC sender to a UDP port for sending messages to the Max/MSP system.
     *
     * @param portNumber the UDP port number to connect to
     * @param IPAddress the IP address to connect to (default is "127.0.0.1")
     */
    void connectOSCSender(int portNumber, juce::String IPAddress);

    /**
     * \brief Attempts to connect an OSC receiver to the specified UDP port.
     * 
     * \param portNumber The port number to attempt the connection on.
     * 
     * If the connection fails, sets currentReceivePort to -1 and logs an error message.
     * If the connection succeeds, sets currentReceivePort to the port number and logs a success message.
     */
    void connectOSCReceiver(int portNumber);

    /**
     * \brief Handles incoming OSC messages by processing them based on their address pattern.
     * 
     * The function checks the OSC address and performs specific actions:
     * - "/loadConfig": Loads configuration from an XML file specified by the message.
     * - "/reset": Resets the system state.
     * - "/setLogname": Sets a new log filename for logging purposes.
     * - "/numIntroTones": Sets the number of introductory tones.
     * - "/plugin": Processes a new user note from an external source, such as Max/MSP.
     * - "/playbackstart": Intended to set a timer at the start of playback, though no longer used.
     * - "/oscstart": Resets the system and sets the processor to manual playing mode.
     * 
     * After processing, sends an action message indicating an OSC message was received.
     * 
     * \param message The OSC message received.
     */
    void oscMessageReceived(const juce::OSCMessage &message);

    int currentReceivePort = -1;

    /**
     * \brief Checks if the OSC receiver is connected.
     * 
     * \return true if the OSC receiver is connected, false otherwise.
     */
    bool isOscReceiverConnected();

    /**
     * \brief Sets the alpha parameters for all player pairs in the processor.
     *
     * \param valueIn The value to set for the alpha parameters.
     */
    void setAlphaBetaParams(float valueIn);

    juce::String GetFileNameOverride();

private:
    std::unique_ptr<Logger> logger;
    //==============================================================================
    int numUserPlayers = 1;
    std::vector<bool> isUserFlags;


    // Previously a local variable in loadMidifile()
    juce::MidiFile midiFile;
    juce::String configSubfolder = "";


    //==============================================================================
    // Timing parameters
    double sampleRate = 44100.0;
    int samplesPerBeat = sampleRate / 4;
    int scoreCounter = 0;

    //==============================================================================
    // Intro countdown
    const int introToneChannel = 16;
    int numIntroTones = 4;
    static const int introToneNoteFirst = 84;
    static const int introToneNoteOther = 72;
    static const juce::uint8 introToneVel = 100;
    int introCounter = 0;
    int introTonesPlayed = 0;
    bool playbackStarted = false;
    bool introFinishedPlaying = false;

    bool firstSampleProcessed = false;

    void playIntroTones(juce::MidiBuffer &midi, int sampleIndex);
    void introToneOn(juce::MidiBuffer &midi, int sampleIndex);
    void introToneOff(juce::MidiBuffer &midi, int sampleIndex);
    // void introToneOnOff (juce::MidiBuffer &midi, juce::MidiMessage (*function)(int, int, juce::uint8), int sampleIndex);
    
    //==============================================================================
    //  Funtions for ammendinding timings for each player in this ensemble. These
    //  should only be called from within processMidiBlock().
    bool initialTempoSet = false;
    // void setTempo (double bpm);

    /**
     * \brief Sets the onset interval for each player in the ensemble to the current tempo.
     *
     * This is only done if the tempo has not yet been set. This is a one-time operation
     * that is used to set the initial tempo for the players. The tempo is set to the
     * current tempo of the processor, which is stored in the samplesPerBeat.
     */
    void setInitialPlayerTempo();

    /**
     * \brief Checks if all players have played a note and thus if
     * there are new onset times available to be used for the next iteration of
     * the ensemble model.
     * 
     * \returns true if all players have played a note, false otherwise
     */
    bool newOnsetsAvailable();

    /**
     * \brief Calculates new onset intervals for each player based on the most recent onset times of the other players and the parameters of the ensemble model.
     *
     * The new onset intervals are calculated by calling recalculateOnsetInterval on each
     * player, and then the next note time is calculated by adding the new interval to
     * the most recent onset time for each player. This information is then sent to the
     * Max/MSP patch through OSC messages. If logging is enabled, the details of the most
     * recent onsets are stored in buffers to be logged.
     */
    void calculateNewIntervals();

    /**
     * \brief Calculates new onset intervals for each player based on the most recent onset times of the other players and the parameters of the ensemble model.
     *
     * The new onset intervals are calculated by calling recalculateOnsetInterval on each
     * player, and then the next note time is calculated by adding the new interval to
     * the most recent onset time for the player. The next note time is then converted
     * to milliseconds and sent to the Max/MSP patch to be displayed.
     *
     * The details of the most recent onsets are then stored in buffers to be logged.
     */
    void clearOnsetsAvailable();

    /**
     * @brief Get the Latest Alphas object (NOT IMPLEMENTED)
     * 
     */
    void getLatestAlphas();

    /**
     * \brief Store the log information about the latest onset from the given player in the
     * logging buffers.
     *
     * \param bufferIndex The index of the logging buffer to store the data in.
     * \param playerIndex The index of the player whose onset details are to be stored.
     */
    void storeOnsetDetailsForPlayer(int playerIndex, Logger::LogData& log);

    //==============================================================================
    // Ensemble players and associated parameters.

    /**
    * \class FlagLock
    * \brief A class to lock a given std::atomic_flag.
    *
    * This class is used to lock the playersInUse flag when we are modifying
    * players. It takes a reference to the flag in its constructor, and
    * atomically tests and sets the flag. If the flag was previously clear, it sets
    * the locked member to true, and if the flag was previously set, it sets the
    * locked member to false.
    *
    * When the object is destroyed, the flag is atomically cleared.
    */
    class FlagLock
    {
    public:
        FlagLock(std::atomic_flag &f);
        ~FlagLock();

        std::atomic_flag &flag;
        bool locked;
    };

    // The following functions should only be called when the playersInUse
    // flag has been locked using the above FlagLock class.
    std::vector<std::unique_ptr<Player>> players;
    std::atomic_flag playersInUse;

    /**
     * \brief Create a Player for each track in the file which has note on events.
     * 
     * The first 'numUserPlayers' will be UserPlayers, and the rest will be
     * Players.
     *
     * \param file The MidiFile from which to create the players
     */
    void createPlayers(const juce::MidiFile &file);

    /**
     * @brief Initialises a matrix of alpha and beta parameters base on the total number of players there are
     * 
     */
    void createAlphaBetaParameters();

    /**
     * \brief Processes a block of MIDI data and updates the state of the EnsembleModel.
     *
     * \param inMidi The input MIDI buffer.
     * \param outMidi The output MIDI buffer.
     * \param sampleIndex The sample index at which the block of MIDI data should be processed.
     */
    void playScore(const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex);
    
    /**
     * \brief Play an intro tone for each user-operated player. 
     * 
     * The intro tone is played at the given
     * sampleIndex, and the note number is given by introToneNoteOther.
     *
     * \param inMidi The input MIDI buffer.
     * \param outMidi The output MIDI buffer.
     * \param sampleIndex The sample index at which to play the intro tone.
     */
    void playUserIntro(const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex);
    
    std::atomic_flag resetFlag;
    
    /**
     * \brief Resets all players in the ensemble model
     * 
     * Initialising intro countdown, score counter and tempo. Also starts loops for logging onset times and polling for
     * new alpha values.
     */
    void resetPlayers();

    /**
     * @brief NOT IMPLEMENTED
     * 
     * @param onsets 
     * @param delays 
     */
    void postLatestOnsets(const std::vector<int> &onsets, const std::vector<int> &delays);

    //==============================================================================
    // Functionality for polling for new alpha values from the server.
    std::unique_ptr<juce::AbstractFifo> pollingFifo;
    std::vector<std::vector<float>> pollingBuffer;

    /**
     * \brief Initialise the polling buffers for the players.
     *
     * Allocates memory for the polling buffers and initialises them to zero. The size of each buffer is 10 times
     * the number of players in the ensemble.
     */
    void initialisePollingBuffers();

    std::thread pollingThread;
    std::atomic<bool> continuePolling;
    std::atomic_flag alphasUpToDate;

    /**
     * \brief Starts the polling loop for the ensemble model.
     *
     * This function stops any existing polling loop, initialises the polling buffers,
     * and starts a new thread to run the polling loop. The polling loop will continue
     * to run until the stopPollingLoop function is called.
     */
    void startPollingLoop();

    /**
     * \brief Stops the polling loop by setting the continuePolling flag to false.
     * 
     * If the polling thread is joinable, it will join the thread to ensure proper cleanup.
     */
    void stopPollingLoop();

    /**
     * \brief Stops the polling loop by setting the continuePolling flag to false.
     * 
     * If the pollingThread is joinable, it will join the thread to ensure
     * proper cleanup and prevent any dangling threads.
     */
    void pollingLoop();

    /**
     * \brief NOT USED
     * 
     */
    void getNewAlphas();

    //==============================================================================
    
    /**
     * \brief Checks if a MIDI sequence contains any note on events.
     *
     * This method can be used to check if a \ref juce::MidiMessageSequence contains any note on events. If it does, the method returns true, otherwise it returns false.
     *
     * \param seq The MIDI sequence to check.
     *
     * \return true if the sequence contains any note on events, false otherwise.
     */
        static bool checkMidiSequenceHasNotes(const juce::MidiMessageSequence *seq);
};
