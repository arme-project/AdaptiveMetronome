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
#include <chrono>

#include "UserPlayer.h"
#include "Player.h"
#include "FlagLock.h"
#include "PluginProcessor.h"

#include "Poller.h"
#include "Logger.h"
#include "ConfigHandler.h"
#include "OSCHandler.h"

using std::function;

class AdaptiveMetronomeAudioProcessor;

using AudioParameterFloatToUse = juce::AudioParameterFloat;

class EnsembleModel
{
public:

	//====================
	//| PUBLIC VARIABLES |
	//====================

	AdaptiveMetronomeAudioProcessor* processor = nullptr; // Pointer to the Adaptive Metronome Processor owner
	bool OSCAutoConnect = true; // Defaulted to autoconnect to port 8000 and 8001
	bool waitingForFirstNote = true;

	// TODO: Change to a JUCE parameter
	juce::Atomic<int> currentNoteIndex;

	//==============================================
	//|CREATION AND DESTRUCTION OF ENSEMBLE OBJECT |
	//==============================================

	EnsembleModel(AdaptiveMetronomeAudioProcessor* processorPtr);
	~EnsembleModel();
	void releaseResources();

	//====================
	//| GETTER FUNCTIONS |
	//====================

	// Getters for Player Parameters
	juce::AudioParameterInt& getPlayerChannelParameter(int playerIndex);
	AudioParameterFloatToUse& getPlayerDelayParameter(int playerIndex);
	AudioParameterFloatToUse& getPlayerMotorNoiseParameter(int playerIndex);
	AudioParameterFloatToUse& getPlayerTimeKeeperNoiseParameter(int playerIndex);
	AudioParameterFloatToUse& getPlayerVolumeParameter(int playerIndex);
	AudioParameterFloatToUse& getAlphaParameter(int player1Index, int player2Index);
	AudioParameterFloatToUse& getBetaParameter(int player1Index, int player2Index);

	// Getters for Ensemble Parameters
	int getNumPlayers();
	int getNumUserPlayers();
	bool isPlayerUserOperated(int playerIndex);
	juce::String GetFileNameOverride();

	// Manual Playing
	bool IsManuallyPlaying();
	void SetManualPlaying(bool isPlaying);

	// Handler Functions
	ConfigHandler* GetConfigHandler() const;
	Logger* GetLogger() const;

	// MIDI file
	juce::MidiFile GetMidiFile();

	//====================
	//| SETTER FUNCTIONS |
	//====================

	void setAlphaBetaParams(float valueIn);
	void SetConfigFileNameOverride(juce::String filename);
	void SetLogSubFolder(juce::String newLogSubFolder);
	void SetNumUserPlayers(int numPlayers);
	void SetNumIntroTones(int numIntroTonesIn);

	void SetTimekeeperNoiseSTD(int index, double value);
	void SetMotorNoiseSTD(int index, double value);
	void SetAlphaParam(int i, int j, double value);
	void SetBetaParam(int i, int j, double value);

	//======================
	//| OPEN SOUND CONTROL |
	//======================

	void setUserOnsetFromOsc(float oscOnsetTime, int onsetNoteNumber, int msMax);
	void ConnectOSCReceiver(int portNumber);
	void SendActionMessage(juce::String message);

	//============================
	//| PREPERATION AND PLAYBACK |
	//============================

	bool loadMidiFile(const juce::File& file, int userPlayers);
	void createPlayers(const juce::MidiFile& file);
	void prepareToPlay(double newSampleRate);

	// Reset functions
	bool reset();
	bool reset(bool skipIntroNotes);

	void setTempo(double bpm);
	static void soundOffAllChannels(juce::MidiBuffer& midi);
	static bool checkMidiSequenceHasNotes(const juce::MidiMessageSequence* seq);

	void triggerFirstNote();

	//=================
	//| MAIN FUNCTION |
	//=================

	void processMidiBlock(const juce::MidiBuffer& inMidi,
		juce::MidiBuffer& outMidi,
		int numSamples,
		double tempo);

private:

	//=====================
	//| PRIVATE VARIABLES |
	//=====================

	// Pointers to corresponding helper classes
	std::unique_ptr<Logger> logger;
	std::unique_ptr<Poller> poller;
	std::unique_ptr<ConfigHandler> config;
	std::unique_ptr<OSCHandler> osc;

	// Defaulted to autoconnect OSC to ports 8000 and 8001
	bool oscAutoConnect = true;

	// File Names and Path
	juce::MidiFile midiFile;
	juce::String configSubfolder = "";

	// Timing Parameters
	double sampleRate = 44100.0;
	int samplesPerBeat = sampleRate / 4;
	int scoreCounter = 0;

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

	// Number of USER players in Ensemble
	int numUserPlayers = 1;
	std::vector<bool> isUserFlags;

	// Intro Tone
	bool initialTempoSet = false;

	//========================
	//| INTRO TONE FUNCTIONS |
	//========================

	void playIntroTones(juce::MidiBuffer& midi, int sampleIndex);
	void introToneOn(juce::MidiBuffer& midi, int sampleIndex);
	void introToneOff(juce::MidiBuffer& midi, int sampleIndex);
	// void introToneOnOff (juce::MidiBuffer &midi, juce::MidiMessage (*function)(int, int, juce::uint8), int sampleIndex);

	//==================
	//| MAIN FUNCTIONS |
	//==================
	//  Funtions for ammendinding timings for each player in this ensemble. These
	//  should only be called from within processMidiBlock().

	void setInitialPlayerTempo();
	bool newOnsetsAvailable();
	void calculateNewIntervals();
	void clearOnsetsAvailable();
	void storeOnsetDetailsForPlayer(int playerIndex, Logger::LogData& log);
	void getLatestAlphas();
	// void setTempo (double bpm);

	//===================================
	//| ENSEMBLE PLAYERS AND PARAMETERS |
	//===================================
	// The following functions should only be called when the playersInUse
	// flag has been locked using the above FlagLock class.

	std::vector<std::unique_ptr<Player>> players;
	std::atomic_flag playersInUse;
	std::atomic_flag resetFlag;

	void createAlphaBetaParameters();
	void playScore(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int sampleIndex);
	void playUserIntro(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int sampleIndex);
	void resetPlayers();
	void postLatestOnsets(const std::vector<int>& onsets, const std::vector<int>& delays);
};
