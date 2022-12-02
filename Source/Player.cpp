#include "Player.h"

//==============================================================================
Player::Player (int index, const juce::MidiMessageSequence *seq, int midiChannel, 
                const double &sampleRate, const int &scoreCounter, int initialInterval)
  : channelParam ("player" + juce::String (index) + "-channel",
                  "Player " + juce::String (index) + " MIDI Channel",
                  1, 16, midiChannel),
    delayParam ("player" + juce::String (index) + "-delay",
                "Player " + juce::String (index) + " Delay",
                0.0, 50.0, 0.0),
    mNoiseStdParam ("player" + juce::String (index) + "-mnoise-std",
                    "Player " + juce::String (index) + " Motor Noise Std",
                    0.0, 10.0, 0.1),
    tkNoiseStdParam ("player" + juce::String (index) + "-tknoise-std",
                     "Player " + juce::String (index) + " Time Keeper Noise Std",
                     0.0, 50.0, 1.0),
    volumeParam ("player" + juce::String (index) + "-volume",
                 "Player " + juce::String (index) + " Volume",
                 0.0, 1.0, 1.0),
    playerIndex (index),
    sampleRate (sampleRate),
    scoreCounter (scoreCounter),
    onsetInterval (initialInterval),
    mNoiseDistribution (0.0, mNoiseStdParam.get() / 1000.0),
    tkNoiseDistribution (0.0, tkNoiseStdParam.get() / 1000.0)
{
    initialiseScore (seq);
}

Player::~Player()
{
}

//==============================================================================
bool Player::isUserOperated()
{
    return false;
}

//==============================================================================
void Player::reset()
{
    // rewind to start of score
    currentNoteIndex = 0;
    
    // reset note on/off counters
    samplesSinceLastOnset = -1;
    samplesToNextOffset = -1;
    
    // reset onset times
    currentOnsetTime = 0;
    previousOnsetTime = 0;
    
    // clear note played flag
    notePlayed = false;
}

//==============================================================================
void Player::setOnsetInterval (int interval)
{
    onsetInterval = interval;
}

int Player::getOnsetInterval()
{
    return onsetInterval;
}

int Player::getPlayedOnsetInterval()
{
    return currentOnsetTime - previousOnsetTime;
}

void Player::recalculateOnsetInterval (int samplesPerBeat,
                                       const std::vector <std::unique_ptr <Player> > &players,
                                       const std::vector <std::unique_ptr <juce::AudioParameterFloat> > &alphas)
{   
    double asyncSum = 0;
        
    for (int i = 0; i < players.size(); ++i)
    {
        double async = currentOnsetTime - players [i]->getLatestOnsetTime();
        asyncSum += *alphas[i] * async;
    }
        
    double hNoise = generateHNoise() * sampleRate;

    onsetInterval = samplesPerBeat - asyncSum + hNoise;
}

//==============================================================================
double Player::generateMotorNoise()
{
    previousMotorNoise = currentMotorNoise;
    
    mNoiseDistribution.param (std::normal_distribution <double>::param_type(0.0, mNoiseStdParam.get() / 1000.0));
    currentMotorNoise = mNoiseDistribution (randomEngine);
    
    return currentMotorNoise;
}

double Player::generateTimeKeeperNoise()
{
    tkNoiseDistribution.param (std::normal_distribution <double>::param_type(0.0, tkNoiseStdParam.get() / 1000.0));
    currentTimeKeeperNoise = tkNoiseDistribution (randomEngine);
    
    return currentTimeKeeperNoise;
}

double Player::generateHNoise()
{
    double mNoise = generateMotorNoise();
    double tkNoise = generateTimeKeeperNoise();
    
    return tkNoise + mNoise - previousMotorNoise;
}

double Player::getMotorNoise()
{
    return currentMotorNoise;
}

double Player::getTimeKeeperNoise()
{
    return currentTimeKeeperNoise;
}

double Player::getMotorNoiseStd()
{
    return mNoiseDistribution.stddev();
}

double Player::getTimeKeeperNoiseStd()
{
    return tkNoiseDistribution.stddev();
}

//==============================================================================
bool Player::hasNotePlayed()
{
    return notePlayed;
}

void Player::resetNotePlayed()
{
    notePlayed = false;
}

int Player::getLatestOnsetTime()
{
    return currentOnsetTime;
}

double Player::getLatestVolume()
{
    return latestVolume;
}

bool Player::wasLatestOnsetUserInput()
{
    return false;
}

//==============================================================================
void Player::processSample (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex)
{
    //==========================================================================
    // Turn off previous note
    if (samplesToNextOffset == 0)
    {
        stopPreviousNote (outMidi, sampleIndex);
    }
        
    if (samplesToNextOffset >= 0)
    {
        --samplesToNextOffset;
    }
    
    //==========================================================================
    // Check if we're at the end of the score.
    if (currentNoteIndex >= notes.size())
    {
        return;
    }
    
    //==========================================================================
    // Do we need to play another note?
    processNoteOn (inMidi, outMidi, sampleIndex);
    
    ++samplesSinceLastOnset;
}

//==============================================================================
std::size_t Player::getNumNotes()
{
    return notes.size();
}

//==============================================================================
void Player::initialiseScore (const juce::MidiMessageSequence *seq)
{
    // Get all note events from MIDI sequence.
    // At the moment note on times are discarded and the notes will be
    // played back in sequence according to timing set by the EnsembleModel.
    // The duration for each note is saved as the time between note on
    // and note off in the MIDI file.
    for (auto event : *seq)
    {
        auto message = event->message;
        
        if (message.isNoteOn())
        {
            auto noteOff = event->noteOffObject->message;
            
            Note note {message.getNoteNumber(),
                       message.getVelocity(),
                       noteOff.getTimeStamp() - message.getTimeStamp()};
                       
            notes.push_back (note);
        }
    }
    
    // Reset to start of score.
    reset();
}

void Player::playNextNote (juce::MidiBuffer &midi, int sampleIndex)
{
    stopPreviousNote (midi, sampleIndex);

    // Add note to buffer.
    auto &note = notes [currentNoteIndex];
    juce::uint8 velocity = note.velocity * volumeParam;
        
    midi.addEvent (juce::MidiMessage::noteOn (channelParam, note.noteNumber, velocity),
                   sampleIndex);
                   
    latestVolume = velocity / 127.0;
            
    // Initialise counters for note timings.
    samplesSinceLastOnset = 0;
    samplesToNextOffset = note.duration * sampleRate;
             
    // Store onset time.
    notePlayed = true;
    previousOnsetTime = currentOnsetTime;
    currentOnsetTime = scoreCounter;
    
    // Move to next note in score.
    ++currentNoteIndex;
}

void Player::stopPreviousNote (juce::MidiBuffer &midi, int sampleIndex)
{
    // Check if notes have been played yet.
    if (currentNoteIndex == 0)
    {
        return;
    }
    
    // Send note off for previous note.
    auto &note = notes [currentNoteIndex - 1];
        
    midi.addEvent (juce::MidiMessage::noteOff (channelParam, note.noteNumber, note.velocity * volumeParam),
                   sampleIndex);
}

void Player::processNoteOn (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex)
{
    if (samplesSinceLastOnset >= onsetInterval || samplesSinceLastOnset < 0)
    {
        playNextNote (outMidi, sampleIndex);
    }
}

//==============================================================================
// Randomness
std::random_device Player::randomSeed;
std::default_random_engine Player::randomEngine (randomSeed());
