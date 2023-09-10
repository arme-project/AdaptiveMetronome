#include "Player.h"
#include "PluginEditor.h"

using namespace std::chrono;
using Editor = AdaptiveMetronomeAudioProcessorEditor;
//==============================================================================
Player::Player (int index, const juce::MidiMessageSequence *seq, int midiChannel, 
                const double &sampleRate, const int &scoreCounter, int initialInterval)
  : channelParam ("player" + juce::String (index) + "-channel",
                  "Player " + juce::String (index) + " MIDI Channel",
                  1, 16, midiChannel),
    delayParam ("player" + juce::String (index) + "-delay",
                "Player " + juce::String (index) + " Delay",
                0.0, 200.0, 0.0),
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
    onsetIntervalForNextNote (initialInterval),
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
    if (logToLogger) {
        juce::String message;
        message << "Player: " << playerIndex;
        Editor::writeToLogger(system_clock::now(), "Player", "Reset", message);
    }
    // rewind to start of score
    currentNoteIndex = 0;
    
    // reset note on/off counters
    samplesSinceLastOnset = 0;
    samplesToNextOffset = -1;
    
    // reset onset times
    currentOnsetTimeInSamples = 0;
    previousOnsetTimeInSamples = 0;
    
    // clear note played flag
    notePlayed = false;
    onsetIntervals.clear();
    onsetTimes.clear();

    //Editor::oscMessageSendNewInterval(playerIndex, currentNoteIndex, 0);

}

void Player::setOscOnsetTime(float onsetFromOsc, int onsetNoteNumber, int samplesSinceFirstNote)
{
    float antescofoDelay = 0.0;
    int antescofoDelaySamples = (int)(antescofoDelay * sampleRate);
    onsetFromOsc -= antescofoDelay;
    samplesSinceFirstNote -= antescofoDelaySamples;
    if (currentNoteIndex >= 0) {
        //DBG("First note received in player");
        oscOnsetTime = onsetFromOsc; // Onset in seconds
        oscOnsetTimeInSamples = samplesSinceFirstNote;
        latestOscOnsetNoteNumber = onsetNoteNumber;
        newOSCOnsetAvailable = true;
        previousOnsetTimeInSamples = currentOnsetTimeInSamples;
        currentOnsetTimeInSamples = oscOnsetTimeInSamples;
        setOnsetIntervalForNextNote(currentOnsetTimeInSamples - previousOnsetTimeInSamples);
    }
    else {
        oscOnsetTime = onsetFromOsc; // Onset in seconds
        oscOnsetTimeInSamples = samplesSinceFirstNote;
        latestOscOnsetNoteNumber = onsetNoteNumber;
        newOSCOnsetAvailable = true;
    }

    if (logToLogger) {
        juce::String message;
        message << "User note number: " << latestOscOnsetNoteNumber << ", "
            << "oscOnsetTime: " << oscOnsetTime << ", "
            << "oscOnsetTimeInSamples: " << oscOnsetTimeInSamples << ", "
            << "oscOnsetTimeConvertToSamples: " << oscOnsetTime / sampleRate;
        Editor::writeToLogger(system_clock::now(), "Player", "setOscOnsetTime", message);
    }
}

//==============================================================================
void Player::setOnsetIntervalForNextNote (int interval)
{
    if (logToLogger) {
        juce::String message;
        message << "Player: " << playerIndex << " : " << interval;
        Editor::writeToLogger(system_clock::now(), "Player", "setOnsetIntervalForNextNote", message);
    }
    onsetIntervalForNextNote = interval;
}

int Player::getOnsetIntervalForNextNote()
{
    return onsetIntervalForNextNote;
}

int Player::getPlayedOnsetIntervalInSamples()
{
    return currentOnsetTimeInSamples - previousOnsetTimeInSamples;
}

void Player::recalculateOnsetInterval (int samplesPerBeat,
                                       const std::vector <std::unique_ptr <Player> > &players,
                                       const std::vector <std::unique_ptr <juce::AudioParameterFloat> > &alphas)
{   
    int previousOnsetInterval = onsetIntervalForNextNote;
    double asyncSum = 0;
        
    for (int i = 0; i < players.size(); ++i)
    {
        double async = currentOnsetTimeInSamples - players [i]->getLatestOnsetTimeInSamples();
        asyncSum += *alphas[i] * async;
    }
        
    double hNoise = generateHNoise() * sampleRate;
    
    setOnsetIntervalForNextNote(samplesPerBeat - asyncSum + hNoise);

    float timeForNextNoteInSamples = currentOnsetTimeInSamples + getOnsetIntervalForNextNote();
    nextNoteTimeInMS = (timeForNextNoteInSamples * 1000.0) / sampleRate;

    if (logToLogger) {
        juce::String message;
        message << "Player: " << playerIndex << " : " << onsetIntervalForNextNote/sampleRate;
        Editor::writeToLogger(system_clock::now(), "Player", "recalculateOnsetInterval", message);
    }
}

int Player::getNextNoteTimeInMS() {
    return nextNoteTimeInMS;
}

void Player::addIntervalToQueue(double interval, double onsetTime)
{
    int maxNumberOfIntervalsInQueue = 19;
    while (numOfIntervalsInQueue = onsetIntervals.size() > maxNumberOfIntervalsInQueue) {
        onsetIntervals.pop_front();
        onsetTimes.pop_front();
    }
    onsetIntervals.push_back(interval);
    onsetTimes.push_back(onsetTime);
    if (logToLogger) {
        juce::String message;
        message << "Player: " << playerIndex << " : " << onsetTime 
            << ", at position: " << onsetIntervals.size();
        Editor::writeToLogger(system_clock::now(), "Player", "addIntervalToQueue", message);
    }
}

void Player::emptyIntervalQueue()
{
    onsetIntervals.empty();
    onsetTimes.empty();
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
    userPlayedNote = false;
}

int Player::getLatestOnsetTimeInSamples()
{
    return currentOnsetTimeInSamples;
}

//int Player::getLatestOnsetTimeInSamples() {
//    
//    return 1;
//
//}

//int Player::getLatestOnsetTimeInSamples() {
//    return currentOnsetTimeInSamples / sampleRate;
//}

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

int Player::getCurrentNoteIndex()
{
    return (int)(currentNoteIndex);
}

//==============================================================================
// Called by EnsembleModel::playScore for each player in turn
// Increments timeSinceLastOnset
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

// Called by processNoteOn
// Adds next midi note to midi buffer
void Player::playNextNote (juce::MidiBuffer &midi, int sampleIndex, int samplesDelay)
{

    stopPreviousNote (midi, sampleIndex);

    // Add note to buffer.
    auto &note = notes [currentNoteIndex];
    if (true) {
    //if (!isUserOperated()) {
            juce::uint8 velocity = note.velocity * volumeParam;

            float floatVelocity;

            // Convert note velocity to float (not sure why this is now needed)
            if (note.velocity > 1) {
                floatVelocity = note.velocity / 127.0;
            }
            else {
                floatVelocity = note.velocity;
            }

            midi.addEvent(juce::MidiMessage::noteOn(channelParam, note.noteNumber, floatVelocity),
                sampleIndex);

            latestVolume = velocity / 127.0;
        //}
    }
            
    // Ignoring delay this onset should have happened samplesDelay samples ago.
    samplesSinceLastOnset = samplesDelay; 

    // 
    samplesToNextOffset = note.duration * sampleRate;
             
    // Store onset time, ignoring per-player delay.
    notePlayed = true;

    // For user player this calculation is performed in setOscOnset
    if (!isUserOperated()) {
        previousOnsetTimeInSamples = currentOnsetTimeInSamples;
        currentOnsetTimeInSamples = scoreCounter - samplesDelay;
    //} else {
        //DBG("NOTE INDEX: " << currentNoteIndex << " : " << latestOscOnsetNoteNumber);
        //DBG("SAMPLES: " << scoreCounter << " : " << oscOnsetTimeInSamples);
        //currentOnsetTimeInSamples = oscOnsetTimeInSamples;
    }
    
    if (logToLogger) {
        juce::String message;
        message << "Player: " << playerIndex << ", Notes played: " << currentNoteIndex << ", "
            << "currentOnsetTime: " << currentOnsetTimeInSamples/sampleRate;
        Editor::writeToLogger(system_clock::now(), "Player", "playNextNote", message);
    }
    // Move to next note in score.
    ++currentNoteIndex;

    if (currentNoteIndex > 1) {
        double onsetIntervalInSamples = getPlayedOnsetIntervalInSamples();
        double onsetIntervalInSeconds = onsetIntervalInSamples / sampleRate;
        double onsetTimeInMS = (currentOnsetTimeInSamples / sampleRate) * 1000.0;
        addIntervalToQueue(onsetIntervalInSeconds, onsetTimeInMS);
    }
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

    float floatVelocity;

    if (note.velocity > 1) {
        floatVelocity = note.velocity / 127;
    }
    else {
        floatVelocity = note.velocity;
    }

    midi.addEvent (juce::MidiMessage::noteOff (channelParam, note.noteNumber, floatVelocity * volumeParam),
                   sampleIndex);
}

// Called by processSample
// Checks if samplesSinceLastOnset has exceeded onsetIntervalForNextNote
void Player::processNoteOn (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex)
{
    int samplesDelay = sampleRate * delayParam / 1000.0;
    
    if (samplesSinceLastOnset >= onsetIntervalForNextNote + samplesDelay || scoreCounter == samplesDelay)
    {
        playNextNote (outMidi, sampleIndex, samplesDelay);
    }
    
    latestDelay = samplesDelay;
}

//==============================================================================
// Randomness
std::random_device Player::randomSeed;
std::default_random_engine Player::randomEngine (randomSeed());
