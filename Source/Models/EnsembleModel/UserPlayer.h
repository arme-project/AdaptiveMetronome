/**
 * \file UserPlayer.h
 * \brief Header file for the UserPlayer class.
 *
 * This file contains the definition of the UserPlayer class, which is a subclass of the Player class.
 * The UserPlayer class is responsible for handling user input and generating MIDI messages based on that input.
 */

#pragma once
#include "Player.h"

 /**
  * \class UserPlayer
  * \brief A player that is operated by user input.
  *
  * The UserPlayer class extends the Player class and is designed to handle user input for MIDI playback.
  * It can process MIDI messages, calculate onset intervals, and manage user-triggered notes.
  */
class UserPlayer : public Player
{
public:
	//==============================================================================

	/**
	 * \brief Constructs a UserPlayer object.
	 *
	 * \param index The index of the player.
	 * \param seq Pointer to a MIDI message sequence.
	 * \param midiChannel The MIDI channel the player should use.
	 * \param sampleRate The audio sample rate.
	 * \param scoreCounter The initial score counter value.
	 * \param initialInterval The initial onset interval for the player.
	 */
	UserPlayer(int index, const juce::MidiMessageSequence* seq, int midiChannel,
		const double& sampleRate, const int& scoreCounter, int initialInterval);

	/**
	 * \brief Constructs a UserPlayer object.
	 *
	 * \param index The index of the player.
	 * \param seq Pointer to a MIDI message sequence.
	 * \param midiChannel The MIDI channel the player should use.
	 * \param sampleRate The audio sample rate.
	 * \param scoreCounter The initial score counter value.
	 * \param initialInterval The initial onset interval for the player.
	 * \param processorIn The audio processor which owns this player.
	 */
	UserPlayer(int index, const juce::MidiMessageSequence* seq, int midiChannel,
		const double& sampleRate, const int& scoreCounter, int initialInterval, AdaptiveMetronomeAudioProcessor* processorIn);

	/**
	 * \brief Destructor for UserPlayer.
	 */
	~UserPlayer();

	//==============================================================================

	/**
	 * \brief Reports this player is user-operated.
	 *
	 * \returns true
	 */
	bool isUserOperated() override;

	//==============================================================================

	/**
	 * \brief Calculates the next onset time for this user player.
	 *
	 * This method works out when the next note should be played, based on the
	 * onsets of the other (non-user) players. If there are no other players, it
	 * simply uses the most recently played interval.
	 *
	 * @param samplesPerBeat The number of samples in a beat.
	 * @param players A vector of all players.
	 */
	void recalculateOnsetInterval(int samplesPerBeat,
		const std::vector<std::unique_ptr<Player>>& players) override;

	//==============================================================================

	/**
	 * \brief Checks if the latest onset was triggered by user input.
	 *
	 * \returns true if the latest note onset was triggered by the user, false otherwise.
	 */
	bool wasLatestOnsetUserInput() override;

	/**
	 * \brief Called from EnsembleModel to play an intro tone when a user presses a key.
	 *
	 * This will play a note at the given introNote at the given sample index, and will
	 * stop this note after the given introToneLength samples. If any note is played by
	 * the user during this time, the intro tone will be cut off.
	 *
	 * \param inMidi The input MIDI buffer.
	 * \param outMidi The output MIDI buffer.
	 * \param sampleIndex The current sample index.
	 * \param introNote The note number to play for the intro tone.
	 */
	void processIntroSample(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int sampleIndex, int introNote) override;

	bool useOSCinput = false;

protected:
	//==============================================================================
	// bool noteTriggeredByUser = false;

	//==============================================================================

	/**
	 * \brief Processes incoming MIDI buffer and outputs a note if the user has inputted a note, or if the user input has timed out.
	 *
	 * If the user has not inputted a note for half an interval length, a note will be played automatically.
	 *
	 * \param inMidi The input MIDI buffer.
	 * \param outMidi The output MIDI buffer.
	 * \param sampleIndex The current sample index.
	 */
	void processNoteOn(const juce::MidiBuffer& inMidi, juce::MidiBuffer& outMidi, int sampleIndex) override;

	int introToneLength = 100;
	int samplesToIntroToneOff = -1;

private:
};
