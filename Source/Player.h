#pragma once
#include <JuceHeader.h>
#include <vector>
#include <random>


/**
 * A class for playing back a sequence of MIDI note on/off events at given intervals.
 */
class Player
{
public:
    //==============================================================================
    Player (int index, const juce::MidiMessageSequence *seq, int midiChannel, 
            const double &sampleRate, const int &scoreCounter, int initialInterval);
    virtual ~Player();
    
    //==============================================================================
    virtual bool isUserOperated();
    
    //==============================================================================
    void reset();
    
    //==============================================================================
    void setOnsetInterval (int interval);
    int getOnsetInterval();
    int getPlayedOnsetInterval();
      
    virtual void recalculateOnsetInterval (int samplesPerBeat,
                                           const std::vector <std::unique_ptr <Player> > &players,
                                           const std::vector <std::unique_ptr <juce::AudioParameterFloat> > &alphas);
    
    //==============================================================================
    double generateMotorNoise();
    double generateTimeKeeperNoise();
    double generateHNoise();
    
    double getMotorNoise();
    double getTimeKeeperNoise();
    
    double getMotorNoiseStd();
    double getTimeKeeperNoiseStd();
    
    //==============================================================================
    bool hasNotePlayed();
    void resetNotePlayed();
    
    int getLatestOnsetTime();
    double getLatestVolume();
    
    //==============================================================================
    void processSample (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex);

    //==============================================================================
    // Parameters
    juce::AudioParameterInt channelParam;
    juce::AudioParameterFloat mNoiseStdParam, tkNoiseStdParam, volumeParam;
    
    //==============================================================================
    std::size_t getNumNotes();
    
protected:
    //==============================================================================
    int playerIndex = 0;
    
    //==============================================================================
    // Score information
    struct Note
    {
        int noteNumber; // MIDI note number
        juce::uint8 velocity; // MIDI velocity
        double duration; // Note duration in seconds
    };
    
    std::vector <Note> notes;
    std::size_t currentNoteIndex = 0;
    double latestVolume = 0.0;
    
    void initialiseScore (const juce::MidiMessageSequence *seq);
    void playNextNote (juce::MidiBuffer &midi, int sampleIndex);
    void stopPreviousNote (juce::MidiBuffer &midi, int sampleIndex);
    
    virtual void processNoteOn (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex);
    
    //==============================================================================
    // Timing information
    const double &sampleRate;
    const int &scoreCounter;
    int onsetInterval = 0; // time between previous and next onset in samples
    
    int samplesSinceLastOnset = -1, samplesToNextOffset = -1;
    
    int currentOnsetTime = 0, previousOnsetTime = 0;
    bool notePlayed = false;
    
private:
    //==============================================================================
    // Randomness
    static std::random_device randomSeed;
    static std::default_random_engine randomEngine;
    std::normal_distribution <double> mNoiseDistribution, tkNoiseDistribution;
    
    double currentMotorNoise = 0.0, previousMotorNoise = 0.0, currentTimeKeeperNoise = 0.0;
};
