#include "PluginProcessor.h"
#include "Player.h"

//==============================================================================
Player::Player (int index, const juce::MidiMessageSequence *seq, int midiChannel, 
                const double &sampleRate, const int &scoreCounter, int initialInterval)
: Player ( index, seq, midiChannel, sampleRate, scoreCounter, initialInterval, nullptr) {}

Player::Player (int index, const juce::MidiMessageSequence *seq, int midiChannel,
                const double &sampleRate, const int &scoreCounter, int initialInterval, AdaptiveMetronomeAudioProcessor *processorPtr)
  : processor(processorPtr),
    playerIndex (index),
    sampleRate (sampleRate),
    scoreCounter (scoreCounter),
    onsetInterval (initialInterval)
{
    processor->channelParameter(playerIndex)->setValueNotifyingHost(playerIndex);
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
    samplesSinceLastOnset = 0;
    samplesToNextOffset = -1;
    
    // reset onset times
    currentOnsetTime = 0;
    previousOnsetTime = 0;
    
    // clear note played flag
    notePlayed = false;
    
    // noises
    currentMotorNoise = 0.0;
    previousMotorNoise = 0.0;
    currentTimeKeeperNoise = 0.0;
    timeKeeperMean = 0.0;
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
                                       const std::vector <std::unique_ptr <juce::AudioParameterFloat> > &alphas,
                                       const std::vector <std::unique_ptr <juce::AudioParameterFloat> > &betas)
{   
    double alphaSum = 0;
    double betaSum = 0;
        
    for (int i = 0; i < players.size(); ++i)
    {
        double async = currentOnsetTime - players [i]->getLatestOnsetTime();
        alphaSum += *alphas[i] * async;
        betaSum += *betas[i] * async;
    }
    
    // update time keeper mean
    timeKeeperMean -= betaSum / sampleRate;
        
    // generate noises for this onset
    double hNoise = generateHNoise() * sampleRate;

    // calcualte next onset interval
    onsetInterval = samplesPerBeat - alphaSum + hNoise;
}

void Player::recalculateOnsetInterval (int samplesPerBeat,
                                       const std::vector <std::unique_ptr <Player> > &players)
{
    double alphaSum = 0;
    double betaSum = 0;
        
    for (int i = 0; i < players.size(); ++i)
    {
        double async = currentOnsetTime - players [i]->getLatestOnsetTime();
        auto alpha = processor->alphaParameter(playerIndex, i)->get();
        auto beta = processor->betaParameter(playerIndex, i)->get();
        alphaSum += alpha * async;
        betaSum += beta * async;
    }
    
    // update time keeper mean
    timeKeeperMean -= betaSum / sampleRate;
        
    // generate noises for this onset
    double hNoise = generateHNoise() * sampleRate;

    // calcualte next onset interval
    onsetInterval = samplesPerBeat - alphaSum + hNoise;
}

//==============================================================================
double Player::generateMotorNoise()
{
    previousMotorNoise = currentMotorNoise;
    //    mNoiseDistribution.param (std::normal_distribution <double>::param_type(0.0, mNoiseStdParam.get() / 1000.0));
    float mNoiseStdParam = processor->mNoiseStdParameter(playerIndex)->get();
    mNoiseDistribution.param (std::normal_distribution <double>::param_type(0.0, mNoiseStdParam / 1000.0));

    currentMotorNoise = mNoiseDistribution (randomEngine);
    
    return currentMotorNoise;
}

double Player::generateTimeKeeperNoise()
{
    //    tkNoiseDistribution.param (std::normal_distribution <double>::param_type(timeKeeperMean, tkNoiseStdParam.get() / 1000.0));
    float tkNoiseStdParam = processor->tkNoiseStdParameter(playerIndex)->get();
    tkNoiseDistribution.param (std::normal_distribution <double>::param_type(timeKeeperMean, tkNoiseStdParam / 1000.0));

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

int Player::getLatestOnsetDelay()
{
    return latestDelay;
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

void Player::playNextNote (juce::MidiBuffer &midi, int sampleIndex, int samplesDelay)
{
    stopPreviousNote (midi, sampleIndex);

    // Add note to buffer.
    auto &note = notes [currentNoteIndex];
    juce::uint8 velocity = note.velocity * processor->volumeParameter(playerIndex)->get();
    int channelParam = processor->channelParameter(playerIndex)->get();
    
    midi.addEvent (juce::MidiMessage::noteOn (channelParam, note.noteNumber, velocity),
                   sampleIndex);
                   
    latestVolume = velocity / 127.0;
            
    // Ignoring delay this onset should have happened samplesDelay samples ago.
    samplesSinceLastOnset = samplesDelay;                    
    samplesToNextOffset = note.duration * sampleRate;
             
    // Store onset time, ignoring per-player delay.
    notePlayed = true;
    previousOnsetTime = currentOnsetTime;
    currentOnsetTime = scoreCounter - samplesDelay;
    
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
    auto channelParam = processor->channelParameter(playerIndex)->get();
    auto volumeParam = processor->volumeParameter(playerIndex)->get();
    midi.addEvent (juce::MidiMessage::noteOff (channelParam, note.noteNumber, note.velocity * volumeParam),
                   sampleIndex);
}

void Player::processNoteOn (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex)
{
    auto delayParam = processor->delayParameter(playerIndex)->get();
    int samplesDelay = sampleRate * delayParam / 1000.0;
    
    if (samplesSinceLastOnset >= onsetInterval + samplesDelay || scoreCounter == samplesDelay)
    {
        playNextNote (outMidi, sampleIndex, samplesDelay);
    }
    
    latestDelay = samplesDelay;
}

//==============================================================================
// Randomness
std::random_device Player::randomSeed;
std::default_random_engine Player::randomEngine (randomSeed());
