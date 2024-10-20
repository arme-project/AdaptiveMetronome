#pragma once
#include <JuceHeader.h>
#include <vector>
#include <atomic>
#include <thread>
#include "Player.h"
#include "OSCManager.h"
#include "XMLManager.h"

using std::function;

class AdaptiveMetronomeAudioProcessor;

class OSCManager;
class XMLManager;

class EnsembleModel :
	private juce::OSCReceiver,
	private juce::OSCReceiver::ListenerWithOSCAddress <juce::OSCReceiver::MessageLoopCallback>,
	public juce::ActionBroadcaster
{
public:
	int numUserPlayers = 1;
	std::vector<std::unique_ptr<Player>> players;
	juce::MidiFile midiFile;

	// Constructor & Destructor
	EnsembleModel();
	EnsembleModel(AdaptiveMetronomeAudioProcessor* processorPtr);
	~EnsembleModel();

	// Initialisation of the Model
	void prepareToPlay(double newSampleRate);
	void releaseResources();
	bool loadMidiFile(const juce::File& file, int userPlayers);
	bool reset();

	// Processing
	void processMidiBlock(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int numSamples, double tempo);

	// Player Management
	int getNumPlayers();
	int getNumUserPlayers();
	bool isPlayerUserOperated(int playerIndex);

	// Getters for Player Parameters
	#pragma region Player Getters
	juce::AudioParameterInt& getPlayerChannelParameter(int playerIndex);
	juce::AudioParameterFloat& getPlayerDelayParameter(int playerIndex);
	juce::AudioParameterFloat& getPlayerMotorNoiseParameter(int playerIndex);
	juce::AudioParameterFloat& getPlayerTimeKeeperNoiseParameter(int playerIndex);
	juce::AudioParameterFloat& getPlayerVolumeParameter(int playerIndex);
	juce::AudioParameterFloat& getAlphaParameter(int player1Index, int player2Index);
	juce::AudioParameterFloat& getBetaParameter(int player1Index, int player2Index);
	#pragma endregion

	// XML Configuration
	#pragma region XML Functions
	void saveConfigToXmlFile();
	void loadConfigFromXml(juce::File configFile);
	#pragma endregion

	// OSC Messaging
	#pragma region OSC Functions
	void connectOSCSender(int portNumber, juce::String IPAddress);
	void connectOSCReceiver(int portNumber);
	void oscMessageReceived(const juce::OSCMessage& message);
	bool isOscReceiverConnected();
	void setAlphaBetaParams(float valueIn);
	int getCurrentReceivePort();
	#pragma endregion

	// Static Utility
	static void soundOffAllChannels(juce::MidiBuffer& midi);

	void createPlayers(const juce::MidiFile& file);
private:
	// Private Members
	std::unique_ptr<OSCManager> oscManager;
	std::unique_ptr<XMLManager> xmlManager;
	AdaptiveMetronomeAudioProcessor* processor = nullptr;

	
	

	// Timing & Score Management
	double sampleRate = 44100.0;
	int samplesPerBeat = sampleRate / 4;
	int scoreCounter = 0;
	bool initialTempoSet = false;

	// Intro Countdown
	const int introToneChannel = 16;
	static const int introToneNoteFirst = 84;
	static const int introToneNoteOther = 72;
	static const juce::uint8 introToneVel = 100;
	int introCounter = 0;
	int introTonesPlayed = 0;

	// Player Initialisation & Playback
	std::atomic_flag playersInUse;
	std::atomic_flag resetFlag;
	
	void createAlphaBetaParameters();
	void resetPlayers();
	void playScore(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int sampleIndex);
	void playUserIntro(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int sampleIndex);

	// Intro Tone Handling
	void playIntroTones(juce::MidiBuffer& midi, int sampleIndex);
	void introToneOn(juce::MidiBuffer& midi, int sampleIndex);
	void introToneOff(juce::MidiBuffer& midi, int sampleIndex);
	void introToneOnOff(juce::MidiBuffer& midi, juce::MidiMessage(*function)(int, int, juce::uint8), int sampleIndex);

	// Timing Adjustments for Players
	void setTempo(double bpm);
	void setInitialPlayerTempo();
	bool newOnsetsAvailable();
	void calculateNewIntervals();
	void clearOnsetsAvailable();
	void getLatestAlphas();
	void storeOnsetDetailsForPlayer(int bufferIndex, int playerIndex);

	// Logging
	struct LogData
	{
		int onsetTime, onsetInterval;
		bool userInput;
		double delay;
		double motorNoise, timeKeeperNoise;
		std::vector<int> asyncs;
		std::vector<float> alphas;
		std::vector<float> betas;
		double tkNoiseStd, mNoiseStd;
		double volume;
	};

	std::vector<LogData> loggingBuffer;
	std::unique_ptr<juce::AbstractFifo> loggingFifo;
	std::thread loggerThread;
	std::atomic<bool> continueLogging;
	int logLineCounter = 0;
	void initialiseLoggingBuffer();
	void startLoggerLoop();
	void stopLoggerLoop();
	void loggerLoop();
	void writeLogHeader(juce::FileOutputStream& logStream);
	void logOnsetDetails(juce::FileOutputStream& logStream);
	void logOnsetDetailsForPlayer(int bufferIndex, juce::String& onsetLog, juce::String& intervalLog,
		juce::String& userInputLog, juce::String& delayLog, juce::String& mNoiseLog,
		juce::String& tkNoiseLog, juce::String& asyncLog, juce::String& alphaLog,
		juce::String& betaLog, juce::String& tkNoiseStdLog, juce::String& mNoiseStdLog,
		juce::String& velocityLog);

	// Posting Onsets
	void postLatestOnsets(const std::vector<int>& onsets, const std::vector<int>& delays);

	// Polling for New Alpha Values
	std::unique_ptr<juce::AbstractFifo> pollingFifo;
	std::vector<std::vector<float>> pollingBuffer;
	std::thread pollingThread;
	std::atomic<bool> continuePolling;
	std::atomic_flag alphasUpToDate;
	void initialisePollingBuffers();
	void startPollingLoop();
	void stopPollingLoop();
	void pollingLoop();
	void getNewAlphas();

	// FlagLock for Player Access
	class FlagLock
	{
	public:
		FlagLock(std::atomic_flag& f);
		~FlagLock();
		std::atomic_flag& flag;
		bool locked;
	};

	// MIDI Sequence Check
	static bool checkMidiSequenceHasNotes(const juce::MidiMessageSequence* seq);

public:
	// Folder and File Management
	juce::String logSubfolder = "";
	juce::String configSubfolder = "";
	juce::String logFilenameOverride = "";

	// Into tones
	int numIntroTones = 4;

	// Parameterised Constructor for Full Initialisation
	EnsembleModel(int numIntroTones, AdaptiveMetronomeAudioProcessor* processor, const juce::String& logSubfolder,
		const juce::String& configSubfolder, const juce::String& logFilenameOverride,
		const juce::OSCSender& OSCSender, int currentReceivePort, int numUserPlayers,
		const juce::MidiFile& midiFile, double sampleRate, int samplesPerBeat, int scoreCounter,
		int introToneChannel, int introCounter, int introTonesPlayed, bool initialTempoSet,
		const std::vector<std::unique_ptr<Player>>& players, const std::atomic_flag& playersInUse,
		const std::atomic_flag& resetFlag, const std::unique_ptr<juce::AbstractFifo>& loggingFifo,
		const std::vector<LogData>& loggingBuffer, const std::thread& loggerThread,
		const std::atomic<bool>& continueLogging, int logLineCounter,
		const std::unique_ptr<juce::AbstractFifo>& pollingFifo,
		const std::vector<std::vector<float>>& pollingBuffer, const std::thread& pollingThread,
		const std::atomic<bool>& continuePolling, const std::atomic_flag& alphasUpToDate);
};
