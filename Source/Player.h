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
    void setOnsetIntervalForNextNote (int interval);
    int getOnsetIntervalForNextNote();
    int getPlayedOnsetIntervalInSamples();
      
    std::deque<double> onsetIntervals;
    std::deque<double> onsetTimes;

    virtual void recalculateOnsetInterval (int samplesPerBeat,
                                           const std::vector <std::unique_ptr <Player> > &players,
                                           const std::vector <std::unique_ptr <juce::AudioParameterFloat> > &alphas);
    
    //EnsembleModel* parentEnsemble;
    //void setParentEnsemble(EnsembleModel* ensemble);


    /**
     * Add an onset interval to the queue. Make sure this is an IOI in seconds.
     */
    void addIntervalToQueue(double interval, double onsetTime);
    void emptyIntervalQueue();
    int numOfIntervalsInQueue = 0;
    //==============================================================================
    // OSC RELATED
    float oscOnsetTime;
    int oscOnsetTimeInSamples;
    int latestOscOnsetNoteNumber;
    void setOscOnsetTime(float onsetFromOsc, int onsetNoteNumber, int samplesSinceFirstNote);
    //void setOscOnsetTimeInSamples(float oscOnsetTime);

    bool newOSCOnsetAvailable = false;

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
    
    int getLatestOnsetDelay();
    double getLatestVolume();
    int getLatestOnsetTimeInSamples();
    virtual bool wasLatestOnsetUserInput();
    int getCurrentNoteIndex();
    int getNextNoteTimeInMS();
    //==============================================================================
    void processSample (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex);

    //==============================================================================
    // Parameters
    juce::AudioParameterInt channelParam;
    juce::AudioParameterFloat delayParam, mNoiseStdParam, tkNoiseStdParam, volumeParam;
    
    //==============================================================================
    std::size_t getNumNotes();

    bool userPlayedNote = false;

protected:
    //==============================================================================
    int playerIndex = 0;
    bool logToLogger = true;
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
    void playNextNote (juce::MidiBuffer &midi, int sampleIndex, int samplesDelay = 0);
    void stopPreviousNote (juce::MidiBuffer &midi, int sampleIndex);
    
    virtual void processNoteOn (const juce::MidiBuffer &inMidi, juce::MidiBuffer &outMidi, int sampleIndex);
    
    //==============================================================================
    // Timing information
    const double &sampleRate;
    const int &scoreCounter;
    int onsetIntervalForNextNote = 0; // time between previous and next onset in samples
    
    int samplesSinceLastOnset = 0, samplesToNextOffset = -1;
    
    int currentOnsetTimeInSamples = 0, previousOnsetTimeInSamples = 0;
    int nextNoteTimeInMS = 0;
    
    int latestDelay = 0;
    bool notePlayed = false;
    
private:
    //==============================================================================
    // Randomness
    static std::random_device randomSeed;
    static std::default_random_engine randomEngine;
    std::normal_distribution <double> mNoiseDistribution, tkNoiseDistribution;
    
    double currentMotorNoise = 0.0, previousMotorNoise = 0.0, currentTimeKeeperNoise = 0.0;
};
