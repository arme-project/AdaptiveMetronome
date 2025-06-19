/**
 * \file Player.h
 * \brief Header file for the Player class.
 *
 * This file contains the definition of the Player class, which is responsible for playing back a sequence of MIDI notes.
 * The Player class handles the timing of the notes, including the onset intervals and the playback of MIDI messages.
 */

#pragma once
#include <JuceHeader.h>
#include <vector>
#include <random>

 /**
  * A class for playing back a sequence of MIDI note on/off events at given intervals.
  */
class AdaptiveMetronomeAudioProcessor;

/**
* \brief The Player class is responsible for playing back a sequence of MIDI notes.
 *
 * The Player class handles the playback of MIDI notes, including the timing of the notes,
 * the onset intervals, and the playback of MIDI messages. It also handles the generation
 * of motor and timekeeper noises, and can be used to play back MIDI sequences in a user-operated
 * or automated manner.
 */
class Player
{
public:
	//==============================================================================
	Player(int index, const juce::MidiMessageSequence* seq, int midiChannel,
		const double& sampleRate, const int& scoreCounter, int initialInterval);

	/**
	 * \brief Constructor for Player.
	 *
	 * \param index The index of the player.
	 * \param seq The midi sequence for the player.
	 * \param midiChannel The MIDI channel the player should use.
	 * \param sampleRate The audio sample rate.
	 * \param scoreCounter The initial score counter value.
	 * \param initialInterval The initial onset interval for the player.
	 * \param processorPtr The audio processor which owns this player.
	 */
	Player(int index, const juce::MidiMessageSequence* seq, int midiChannel,
		const double& sampleRate, const int& scoreCounter, int initialInterval, AdaptiveMetronomeAudioProcessor* processorPtr);

	/**
	 * \brief Destructor for Player.
	 */
	virtual ~Player();

	AdaptiveMetronomeAudioProcessor* processor;

	//==============================================================================

	/**
	 * \brief Reports whether this player is user-operated.
	 *
	 * \returns false
	 */
	virtual bool isUserOperated();

	//==============================================================================

	/**
	 * \brief Resets the player to the start of the score.
	 *
	 * Sets the current note index to 0, resets the note on/off counters, resets
	 * the onset times, clears the note played flag, and clears the onset intervals and
	 * times vectors. Also resets the motor and timekeeper noises.
	 */
	void reset();

	//==============================================================================

	/**
	 * \brief Sets the onset interval for the player.
	 *
	 * Sets the onset interval for the player in samples. This is the time interval
	 * between the current onset and the next onset. This is used to determine when
	 * the next note should be played.
	 *
	 * \param interval The onset interval in samples.
	 */
	void setOnsetInterval(int interval);

	/**
	 * \brief Returns the current onset interval for the player.
	 *
	 * This function retrieves the time interval in samples between the current
	 * onset and the next onset. It is used to determine the timing for the next
	 * note to be played.
	 *
	 * \returns The onset interval in samples.
	 */
	int getOnsetInterval();

	/**
	 * \brief Returns the onset interval for the last note played.
	 *
	 * This function retrieves the time interval in samples between the current
	 * onset and the previous onset. It is used to determine the timing for the
	 * previous note that was played.
	 *
	 * \returns The onset interval in samples.
	 */
	int getPlayedOnsetInterval();

	std::deque<double> onsetIntervals;
	std::deque<double> onsetTimes;

	/**
	 * \brief Recalculates the next onset interval for the player.
	 *
	 * This function updates the time keeper mean and generates noises for the current
	 * onset. It then calculates the next onset interval based on the alpha and beta
	 * parameters, the current onset time and the latest onset times of the other
	 * players.
	 *
	 * \param samplesPerBeat The number of samples per beat.
	 * \param players The list of other players.
	 */
	virtual void recalculateOnsetInterval(int samplesPerBeat,
		const std::vector<std::unique_ptr<Player>>& players);

	/**
	 * \brief Add an onset interval to the queue. Make sure this is an IOI in seconds.
	 */
	void addIntervalToQueue(double interval, double onsetTime);

	/**
	 * \brief Empty the interval queue.
	 *
	 */
	void emptyIntervalQueue();

	int numOfIntervalsInQueue = 0;

	//==============================================================================
	// OSC RELATED
	float oscOnsetTime;
	int oscOnsetTimeInSamples;
	int latestOscOnsetNoteNumber;

	/**
	 * \brief This method is used by the UserPlayer to set the onset time from an OSC message.
	 * It sets newOSCOnsetAvailable to true, and sets the onset time in samples.
	 * newOSCOnsetAvailable is checked in UserPlayer::processNoteOn. This should be
	 * called when a note is played from Max via OSC.
	 *
	 * Called from EnsembleModel::setUserOnsetFromOsc.
	 *
	 * \param onsetFromOsc The onset time from the OSC message, in seconds.
	 * \param onsetNoteNumber The note number of the note that was played.
	 * \param samplesSinceFirstNote The number of samples since the first note was played.
	 */
	void setOscOnsetTime(float onsetFromOsc, int onsetNoteNumber, int samplesSinceFirstNote);
	// void setOscOnsetTimeInSamples(float oscOnsetTime);

	bool newOSCOnsetAvailable = false;

	//==============================================================================
	// GETTERS FOR NOISES
	double generateMotorNoise();
	double generateTimeKeeperNoise();
	double generateHNoise();

	double getMotorNoise();
	double getTimeKeeperNoise();

	double getMotorNoiseStd();
	double getTimeKeeperNoiseStd();

	//==============================================================================

	/**
	 * Returns true if the player has played a note in the most recent process block
	 */
	bool hasNotePlayed();

	/**
	 * Resets the note played flag to indicate that no note has been played.
	 */
	void resetNotePlayed();

	/**
	 * Returns the latest onset time in samples.
	 */
	int getLatestOnsetTime();

	/**
	 * Returns the latest onset delay in samples.
	 */
	int getLatestOnsetDelay();

	/**
	 * Returns the latest volume of the note played.
	 */
	double getLatestVolume();

	/**
	 * Returns whether the latest onset of this player was caused by user input.
	 */
	virtual bool wasLatestOnsetUserInput();

	/**
	 * \brief Returns the index of the note which is currently being played.
	 */
	int getCurrentNoteIndex();

	// int getNextNoteTimeInMS();
	//==============================================================================

	/**
	 * \brief This method is responsible for generating the audio output for one sample of the MIDI buffer.
	 *
	 * It checks whether the previous note has finished, and if so,
	 * plays the next note (if there is one). If not, it just decrements
	 * samplesToNextOffset to move closer to playing the next note.
	 *
	 * @param inMidi The input MIDI buffer.
	 * @param outMidi The output MIDI buffer.
	 * @param sampleIndex The index of the sample to process in the buffers.
	 */
	void processSample(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int sampleIndex);

	/**
	 * * \brief This method is responsible for processing the intro sample for one sample of the MIDI buffer.
	 */
	virtual void processIntroSample(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int sampleIndex, int introNote) {};

	//==============================================================================
	// Parameters - Moved to processor
	//    juce::AudioParameterInt channelParam;
	//    juce::AudioParameterFloat delayParam, mNoiseStdParam, tkNoiseStdParam, volumeParam;

	//==============================================================================

	/**
	 * \brief Returns the number of notes in the player's score.
	 *
	 * This function counts the total number of notes that have been
	 * initialized and are available in the player's note sequence.
	 *
	 * \returns The size of the notes vector, representing the total number
	 *          of notes in the sequence.
	 */
	std::size_t getNumNotes();

protected:
	//==============================================================================
	int playerIndex = 0;

	//==============================================================================
	// Score information
	struct Note
	{
		int noteNumber;       // MIDI note number
		juce::uint8 velocity; // MIDI velocity
		double duration;      // Note duration in seconds
	};

	std::vector<Note> notes;
	std::size_t currentNoteIndex = 0;
	double latestVolume = 0.0;

	/**
	 * \brief Loads a MIDI sequence into the player.
	 *
	 * This method loads the note numbers, velocities and durations from a MIDI
	 * sequence into the player. Note on times are discarded and the notes will be
	 * played in sequence according to timing set by the EnsembleModel.
	 *
	 * \param seq The MIDI sequence to load.
	 */
	void initialiseScore(const juce::MidiMessageSequence* seq);

	/**
	 * \brief Plays the next note in the score.
	 *
	 * Adds the next note in the score to the midi output stream.
	 *
	 * \param midi The midi output stream.
	 * \param sampleIndex The sample index at which the note should be added.
	 * \param samplesDelay The number of samples the note should be delayed by.
	 */
	void playNextNote(juce::MidiBuffer& midi, int sampleIndex, int samplesDelay = 0);

	/**
	 * \brief Stops the previous note in the score being played.
	 *
	 * This function sends a midi note off message for the note that was
	 * previously played. It is called from processSample.
	 *
	 * \param midi The output midi buffer.
	 * \param sampleIndex The current sample index.
	 */
	void stopPreviousNote(juce::MidiBuffer& midi, int sampleIndex);

	/**
	 * \brief Processes a single sample of incoming MIDI data for a player.
	 *
	 * This method is called from processSample. It checks if the player should play a new note
	 * based on the onset interval and the delay (if any) set for the player. If the player should
	 * play a note, it calls playNextNote to add the note to the output MIDI buffer.
	 *
	 * \param inMidi The incoming MIDI data.
	 * \param outMidi The output MIDI data.
	 * \param sampleIndex The sample index of the incoming MIDI data.
	 */
	virtual void processNoteOn(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int sampleIndex);
	//==============================================================================
	// Timing information
	const double& sampleRate;
	const int& scoreCounter;
	int onsetInterval = 0; // time between previous and next onset in samples

	int samplesSinceLastOnset = 0, samplesToNextOffset = -1;

	int currentOnsetTime = 0, previousOnsetTime = 0;
	int nextNoteTimeInMS = 0;

	int latestDelay = 0;
	bool notePlayed = false;

private:
	//==============================================================================
	// Randomness
	static std::random_device randomSeed;
	static std::default_random_engine randomEngine;
	std::normal_distribution<double> mNoiseDistribution, tkNoiseDistribution;

	double currentMotorNoise = 0.0, previousMotorNoise = 0.0, currentTimeKeeperNoise = 0.0, timeKeeperMean = 0.0;

protected:
	bool noteTriggeredByUser = false;
};
