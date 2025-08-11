/*
  ==============================================================================

    AudioPlaybackController.h
    Created: 5 Aug 2025 12:25:24pm
    Author:  User

  ==============================================================================
*/
#pragma once
#include <JuceHeader.h>
#include <rubberband/RubberBandStretcher.h>

// Forward declare processor as this requires a pointer to it
class AdaptiveMetronomeAudioProcessor; // Forward declaration

// TODO: Sample Rate
// TODO: Time stretching ratio calculation

/**
 * \brief This class will define a wrapper around an audio file and an instance of RubberBand.
 *
 * This will keep track of how many samples have been sent to RB, what time ratio was used, and how many have been retrieved for audio output.
 *
 * This allows more precise awareness of where we are in playback, how much of the audio file has already been processed for stretching, and what time ratio is required for a particular sample (note onset) to be played at a particular actual time.
 *
 * All calculations are performed in samples (with conversion functions provided to get actual time in seconds)

 */
class AudioPlaybackController
{
public:
    // ----------------------------------------------------------------------------
    // Constructor and Destructor
    // ----------------------------------------------------------------------------
    AudioPlaybackController(AdaptiveMetronomeAudioProcessor *processorPtr);
    ~AudioPlaybackController();

    // ----------------------------------------------------------------------------
    // Core Public Interface
    // ----------------------------------------------------------------------------
    void loadFile(const juce::File &file);
    void StartStretcher();
    void StopStretcher();
    void ProcessBlock(juce::AudioBuffer<float> &buffer);
    void start();
    
    // ----------------------------------------------------------------------------
    // Thread-Safe Getters
    // ----------------------------------------------------------------------------
    int getPlaybackSample() const;
    int getStretchThreadCounter() const;
    int getAudioThreadCounter() const;
    bool isStretcherRunning() const;
    bool isCurrentlyPlaying() const;
    int getAvailableStretchedSamplesInBuffer() const;
    
    // ----------------------------------------------------------------------------
    // Setters with Validation
    // ----------------------------------------------------------------------------
    void setPlaybackSample(int sample);
    void incrementAudioThreadCounter();

private:
    // ----------------------------------------------------------------------------
    // Core Components (Private)
    // ----------------------------------------------------------------------------
    AdaptiveMetronomeAudioProcessor *processorPtr;
    RubberBand::RubberBandStretcher stretcher;
    std::thread stretchThread;
    
    // ----------------------------------------------------------------------------
    // State Variables (Private)
    // ----------------------------------------------------------------------------
    mutable std::mutex stateMutex; // For thread-safe access to counters/state
    bool isStretchThreadRunning = false;
    bool stretchRequiresFirstPad = true;
    bool isCurrentlyPlayingFlag = false;
    int playbackSample = 0;
    int stretchThreadCounter = 0;
    int audioThreadCounter = 0;
    
    // ----------------------------------------------------------------------------
    // Audio Data (Private)
    // ----------------------------------------------------------------------------
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioBuffer<float> audioWavBuffer;
    std::vector<float> rubberBandInputBuffer;
    std::vector<float> rubberBandOutputBuffer;
    
    // ----------------------------------------------------------------------------
    // Private Implementation Methods
    // ----------------------------------------------------------------------------
    RubberBand::RubberBandStretcher::Options getOptions();
    void StretcherLoop();
    int retrieveSamples();

public:
    // ----------------------------------------------------------------------------
    // Public Nested Classes (Controlled Access)
    // ----------------------------------------------------------------------------

    // ----------------------------------------------------------------------------
    // Public Nested Classes (Controlled Access)
    // ----------------------------------------------------------------------------
    
    struct TimeStretchEntry
    {
        int nAvailableSamples = 0;
        int nPlaybackSamples = 0;
        int nPlaybackSamplesCalculated = 0;
        int playbackSamplesAtStart = 0;
        int playbackSamplesAtEnd = 0;
        int totalAvailableSamplesAdded = 0;
        float timeRatio = 0.0f;
        float calculatedRatio = 0.0f;

        TimeStretchEntry();
        void reset();
        int setPlaybackSamples(int start, int end, int nPlaybackSamplesIn);
        int playbackSamplesInEntry();
        float inputTimeRatio() const;
        float calculateTimeRatio();
        size_t checkConsistency();
        TimeStretchEntry(const TimeStretchEntry &other);
    };

    class TimeStretchLogBuffer
    {
    public:
        std::deque<TimeStretchEntry> availableStretchedEntries;
        std::deque<TimeStretchEntry> playedStretchedEntries;
        std::vector<float> &rubberBandOutputBuffer;

        int totalNumberOfSamplesInOutputBuffer = 0;
        
        // Public mutex for external access
        std::mutex rubberBandOutputBufferMutex; 

        TimeStretchLogBuffer(std::vector<float> &outputBuffer);

        void addEntry(TimeStretchEntry newEntry);
        void addPlayedEntry(TimeStretchEntry playedEntry);
        void removeSamplesSynchronously(int samplesToRemove);
        void removeSamplesFromOutputBuffer(int samplesToRemove);
        void removeSamplesFromLog(int samplesToRemove);

    private:
        // Mutex for thread safety
        std::mutex availableBufferMutex; 
        std::mutex playedBufferMutex; 
    };

    // ----------------------------------------------------------------------------
    // Public Members (Controlled Access)
    // ----------------------------------------------------------------------------
    TimeStretchEntry tracker;
    TimeStretchLogBuffer stretchTrackerLog;
};

// ----------------------------------------------------------------------------
// Now that I have the ability to load a wav file, and control the timestretching when it is played back, I would like to extend this to my actual use case. I would like to create a new data structure, and the ability to serialize and load it from an xml file. These files would store the data needed to load a single musical piece into the plugin. This would typically be a classical piece of music, with 2 to 4 players. The user would typically take the role of one of the instruments, while the other three need to be loaded as virtual players. Much of this functionality is already built in another version of this plugin, however at the moment it loads only a MIDI file, with the required score for each player. The sound is then reproduced from the MIDI score. I would like to extend this to load an associated .wav file for each player, and