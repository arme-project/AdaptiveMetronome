
/*
  ==============================================================================

    AudioPlaybackController.cpp
    Created: 5 Aug 2025 12:25:24pm
    Author:  User

  ==============================================================================
*/

#include "AudioPlaybackController.h"
#include <rubberband/RubberBandStretcher.h>
#include "PluginProcessor.h"

using RBOpt = RubberBand::RubberBandStretcher::Option;

// ============================================================================
// AudioPlaybackController Implementation
// ============================================================================

// ----------------------------------------------------------------------------
// Constructor and Destructor
// ----------------------------------------------------------------------------

AudioPlaybackController::AudioPlaybackController(AdaptiveMetronomeAudioProcessor *processorPtr)
    : processorPtr(processorPtr),
      stretcher(44100, 1, getOptions()),
      stretchTrackerLog(rubberBandOutputBuffer)
{
    formatManager.registerBasicFormats();
}

AudioPlaybackController::~AudioPlaybackController()
{
    // Ensure proper cleanup
    StopStretcher();
}

// ----------------------------------------------------------------------------
// Thread-Safe Getters
// ----------------------------------------------------------------------------

int AudioPlaybackController::getPlaybackSample() const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return playbackSample;
}

int AudioPlaybackController::getStretchThreadCounter() const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return stretchThreadCounter;
}

int AudioPlaybackController::getAudioThreadCounter() const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return audioThreadCounter;
}

bool AudioPlaybackController::isStretcherRunning() const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return isStretchThreadRunning;
}

bool AudioPlaybackController::isCurrentlyPlaying() const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return isCurrentlyPlayingFlag;
}

int AudioPlaybackController::getAvailableStretchedSamplesInBuffer() const
{
    return static_cast<int>(rubberBandOutputBuffer.size());
}

// ----------------------------------------------------------------------------
// Setters with Validation
// ----------------------------------------------------------------------------

void AudioPlaybackController::setPlaybackSample(int sample)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    playbackSample = std::max(0, sample);
}

void AudioPlaybackController::incrementAudioThreadCounter()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    audioThreadCounter++;
}

// ----------------------------------------------------------------------------
// Public Interface Methods
// ----------------------------------------------------------------------------

void AudioPlaybackController::loadFile(const juce::File &file)
{
    auto *reader = formatManager.createReaderFor(file);
    if (reader != nullptr)
    {
        audioWavBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
        if (!(reader->read(&audioWavBuffer, 0, (int)reader->lengthInSamples, 0, true, true)))
        {
            DBG("FAILED TO READ SAMPLES FROM WAV");
        }
    }
}

void AudioPlaybackController::StartStretcher()
{
    DBG("START STRETCHER ... ");
    stretcher.reset();
    stretchRequiresFirstPad = true;
    auto currentTimeRatio = processorPtr->apvts.getRawParameterValue("timeRatio")->load();
    stretcher.setTimeRatio(currentTimeRatio);
    StopStretcher();
    
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        stretchThreadCounter = 0;
        audioThreadCounter = 0;
        isStretchThreadRunning = true;
    }
    
    stretchTrackerLog.totalNumberOfSamplesInOutputBuffer = 0;
    stretchThread = std::thread([this]()
                                { this->StretcherLoop(); });
}

void AudioPlaybackController::StopStretcher()
{
    DBG("STOPPING STRETCHER");
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        isStretchThreadRunning = false;
    }
    if (stretchThread.joinable())
        stretchThread.join();
    stretcher.reset();
}

void AudioPlaybackController::start()
{
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        isCurrentlyPlayingFlag = true;
    }
    StartStretcher();
}

void AudioPlaybackController::ProcessBlock(juce::AudioBuffer<float> &buffer)
{
    // Get number of samples needed
    auto bufferSize = buffer.getNumSamples();

    // Copy the processed samples from the RubberBand output buffer to the provided buffer
    std::copy(rubberBandOutputBuffer.begin(),
        rubberBandOutputBuffer.begin() + bufferSize,
        buffer.getWritePointer(0));

    // Remove the processed samples from the tracker log and output buffer
    stretchTrackerLog.removeSamplesSynchronously(bufferSize);
}

int AudioPlaybackController::retrieveSamples()
{
    int samplesToCopy = stretcher.available();
    if (samplesToCopy <= 0)
        return 0;
    std::lock_guard<std::mutex> lock(stretchTrackerLog.rubberBandOutputBufferMutex);
    rubberBandOutputBuffer.resize(rubberBandOutputBuffer.size() + samplesToCopy);
    float* outputChannels[] = { rubberBandOutputBuffer.data() + rubberBandOutputBuffer.size() - samplesToCopy };
    stretcher.retrieve(outputChannels, samplesToCopy);
    return samplesToCopy;
}

// ----------------------------------------------------------------------------
// Private Implementation Methods
// ----------------------------------------------------------------------------

RubberBand::RubberBandStretcher::Options AudioPlaybackController::getOptions()
{
    RubberBand::RubberBandStretcher::Options options =
        RBOpt::OptionProcessRealTime |
        RBOpt::OptionEngineFaster;

    return options;
}

void AudioPlaybackController::StretcherLoop()
{
    DBG("STARTING STRETCHER LOOP ...");
    while (isStretcherRunning())
    {
        // Get number of required samples and number of samples in OutputBuffer
        auto samplesRequired = stretcher.getSamplesRequired();
        auto numberOfSamplesInOutputBuffer = rubberBandOutputBuffer.size();
        
        // If there are already enough samples available, skip processing
        // TODO: Decide what threshold to use?
        if (numberOfSamplesInOutputBuffer > 480 * 3 || samplesRequired <= 0)
        {
            // Sleep thread for 1ms
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        // Increment stretchThreadCounter only if we're processing more samples
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            stretchThreadCounter++;
        }

        // Update tracker with initial values
        tracker.reset();
        auto playbackSamplesAtStart = getPlaybackSample();

        // TODO: More robust checks on whether timeRatio needs changing. 
        // Going forward, the ratio will not actually be controlled by the audio parameter.
        tracker.timeRatio = processorPtr->apvts.getRawParameterValue("timeRatio")->load();

        // Update timeRatio in stretcher if needed
        stretcher.setTimeRatio(tracker.timeRatio);

        // Get preferred StartPad if needed
        // TODO: Currently has copied code. If statement should include only padding code. Copying should be done the same in both scenarios. 
        rubberBandInputBuffer.resize(samplesRequired);
        if (stretchRequiresFirstPad)
        {
            // Get preferred StartPad if needed
            auto paddingSamples = stretcher.getPreferredStartPad();
            // Fill the input buffer with zeros for the padding samples
            std::fill(rubberBandInputBuffer.begin(), rubberBandInputBuffer.begin() + paddingSamples, 0.0f);
            // Set stretchRequiresFirstPad to false for subsequent iterations
            stretchRequiresFirstPad = false;
            // Get the source pointer and copy the remaining required samples into the input buffer
            const float *sourcePtr = audioWavBuffer.getReadPointer(0, getPlaybackSample());
            auto remainingSamples = rubberBandInputBuffer.size() - paddingSamples;
            std::copy(sourcePtr,
                      sourcePtr + remainingSamples,
                      rubberBandInputBuffer.begin() + paddingSamples);

            // Update playbackSample to account for the padding samples
            setPlaybackSample(getPlaybackSample() + remainingSamples);
        }
        else
        {
            // If no padding is required, just copy the samples directly
            // Get the source pointer and copy the required samples into the input buffer
            const float *sourcePtr = audioWavBuffer.getReadPointer(0, getPlaybackSample());
            int remainingSamples = rubberBandInputBuffer.size();
            int totalSamplesRemainingInWavBuffer = audioWavBuffer.getNumSamples() - getPlaybackSample();
            // If there are not enough samples remaining in the wav buffer, skip processing
            // TODO: Replace this with padding logic if needed
            if (totalSamplesRemainingInWavBuffer < remainingSamples)
            {
                continue;
            }
            // Copy the samples from the source pointer to the input buffer
            std::copy(sourcePtr,
                      sourcePtr + remainingSamples,
                      rubberBandInputBuffer.begin());

            setPlaybackSample(getPlaybackSample() + remainingSamples);
        } // Finish setting up input buffer
        auto playbackSamplesAdded = getPlaybackSample() - playbackSamplesAtStart;
        tracker.setPlaybackSamples(playbackSamplesAtStart, getPlaybackSample(), playbackSamplesAdded);
        
        // Process the input buffer with RubberBand
        const float *inputChannels[] = {rubberBandInputBuffer.data()};
        stretcher.process(inputChannels, samplesRequired, false);
        
        // Copy processed samples to the stretched sample buffer
        tracker.nAvailableSamples = retrieveSamples();
        stretchTrackerLog.addEntry(tracker);
    }
}

// ============================================================================
// TimeStretchEntry Implementation
// ============================================================================

// TimeStretchEntry Implementation
AudioPlaybackController::TimeStretchEntry::TimeStretchEntry() = default;

void AudioPlaybackController::TimeStretchEntry::reset()
{
    nAvailableSamples = 0;
    nPlaybackSamples = 0;
    nPlaybackSamplesCalculated = 0;
    playbackSamplesAtStart = 0;
    playbackSamplesAtEnd = 0;
    totalAvailableSamplesAdded = 0;
    timeRatio = 0.0f;
    calculatedRatio = 0.0f;
}

int AudioPlaybackController::TimeStretchEntry::setPlaybackSamples(int start, int end, int nPlaybackSamplesIn)
{
    nPlaybackSamples = nPlaybackSamplesIn;
    playbackSamplesAtStart = start;
    playbackSamplesAtEnd = end;
    return playbackSamplesInEntry();
}

int AudioPlaybackController::TimeStretchEntry::playbackSamplesInEntry()
{
    nPlaybackSamplesCalculated = (int)playbackSamplesAtEnd - (int)playbackSamplesAtStart;
    return nPlaybackSamples;
}

float AudioPlaybackController::TimeStretchEntry::inputTimeRatio() const
{
    return timeRatio;
}

float AudioPlaybackController::TimeStretchEntry::calculateTimeRatio()
{
    calculatedRatio = static_cast<float>(nAvailableSamples) / static_cast<float>(nPlaybackSamples);
    return calculatedRatio;
}

AudioPlaybackController::TimeStretchEntry::TimeStretchEntry(const TimeStretchEntry &other)
    : nAvailableSamples(other.nAvailableSamples),
      nPlaybackSamples(other.nPlaybackSamples),
      nPlaybackSamplesCalculated(other.nPlaybackSamplesCalculated),
      playbackSamplesAtStart(other.playbackSamplesAtStart),
      playbackSamplesAtEnd(other.playbackSamplesAtEnd),
      totalAvailableSamplesAdded(other.totalAvailableSamplesAdded),
      timeRatio(other.timeRatio),
      calculatedRatio(other.calculatedRatio)
{
    playbackSamplesInEntry();
    calculateTimeRatio();
}

// ============================================================================
// TimeStretchLogBuffer Implementation
// ============================================================================

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------

AudioPlaybackController::TimeStretchLogBuffer::TimeStretchLogBuffer(std::vector<float> &outputBuffer)
    : rubberBandOutputBuffer(outputBuffer)
{
    availableStretchedEntries.clear(); // Set size to 0 until entries are actually added
    playedStretchedEntries.clear();
}

// ----------------------------------------------------------------------------
// Entry Management Methods
// ----------------------------------------------------------------------------

void AudioPlaybackController::TimeStretchLogBuffer::addEntry(TimeStretchEntry newEntry)
{
    totalNumberOfSamplesInOutputBuffer += newEntry.nAvailableSamples;
    jassert(totalNumberOfSamplesInOutputBuffer == rubberBandOutputBuffer.size());
    std::lock_guard<std::mutex> lock(availableBufferMutex);
    if (availableStretchedEntries.empty())
    {
        newEntry.totalAvailableSamplesAdded = newEntry.nAvailableSamples;
        availableStretchedEntries.push_back(std::move(newEntry));
        if (availableStretchedEntries.back().nPlaybackSamples != availableStretchedEntries.back().nPlaybackSamplesCalculated)
        {
            DBG("WARNING: DISCREPANCY IN PLAYBACK SAMPLES AFTER NEW");
        }
        return;
    }
    TimeStretchEntry &lastEntry = availableStretchedEntries.back();
    if (lastEntry.timeRatio != newEntry.timeRatio)
    {
        newEntry.totalAvailableSamplesAdded = newEntry.nAvailableSamples;
        availableStretchedEntries.push_back(std::move(newEntry));
        if (availableStretchedEntries.back().nPlaybackSamples != availableStretchedEntries.back().nPlaybackSamplesCalculated)
        {
            DBG("WARNING: DISCREPANCY IN PLAYBACK SAMPLES AFTER NEW TR");
        }
    }
    else
    {
        lastEntry.nPlaybackSamples += newEntry.nPlaybackSamples;
        lastEntry.playbackSamplesAtEnd = newEntry.playbackSamplesAtEnd;
        lastEntry.totalAvailableSamplesAdded += newEntry.nAvailableSamples;
        lastEntry.nAvailableSamples += newEntry.nAvailableSamples;
        lastEntry.playbackSamplesInEntry();
        if (availableStretchedEntries.back().nPlaybackSamples != availableStretchedEntries.back().nPlaybackSamplesCalculated)
        {
            DBG("WARNING: DISCREPANCY IN PLAYBACK SAMPLES AFTER ADD");
        }
    }
}

void AudioPlaybackController::TimeStretchLogBuffer::addPlayedEntry(TimeStretchEntry playedEntry)
{
    std::lock_guard<std::mutex> lock(playedBufferMutex);
    playedStretchedEntries.push_back(std::move(playedEntry));
}

// ----------------------------------------------------------------------------
// Sample Removal Methods
// ----------------------------------------------------------------------------

void AudioPlaybackController::TimeStretchLogBuffer::removeSamplesSynchronously(int samplesToRemove)
{
    totalNumberOfSamplesInOutputBuffer -= samplesToRemove;
    removeSamplesFromOutputBuffer(samplesToRemove);
    std::unique_lock<std::mutex> lock(availableBufferMutex);
    removeSamplesFromLog(samplesToRemove);
    jassert(totalNumberOfSamplesInOutputBuffer == rubberBandOutputBuffer.size());
}

void AudioPlaybackController::TimeStretchLogBuffer::removeSamplesFromOutputBuffer(int samplesToRemove)
{
    std::lock_guard<std::mutex> lock(rubberBandOutputBufferMutex);
    if (samplesToRemove > rubberBandOutputBuffer.size())
    {
        samplesToRemove = rubberBandOutputBuffer.size();
    }
    rubberBandOutputBuffer.erase(rubberBandOutputBuffer.begin(),
                                rubberBandOutputBuffer.begin() + samplesToRemove);
}

void AudioPlaybackController::TimeStretchLogBuffer::removeSamplesFromLog(int samplesToRemove)
{
    if (availableStretchedEntries.empty())
    {
        DBG("WARNING: TRYING TO REMOVE SAMPLES FROM EMPTY LOG BUFFER");
        return;
    }
    int whichEntry = 0;
    for (int i = 0; i < availableStretchedEntries.size(); ++i)
    {
        TimeStretchEntry& checkEntry = availableStretchedEntries[i];
        if (checkEntry.nAvailableSamples > 0)
        {
            whichEntry = i;
            break;
        }
    }
    TimeStretchEntry& checkEntry = availableStretchedEntries[whichEntry];
    jassert(checkEntry.nAvailableSamples > 0);
    int removableSamples = std::min(samplesToRemove, checkEntry.nAvailableSamples);
    if (removableSamples > 0)
    {
        float proportionOfAvailableSamplesToRemove =
            static_cast<float>(removableSamples) / static_cast<float>(checkEntry.nAvailableSamples);
        checkEntry.nAvailableSamples -= removableSamples;
        auto numberOfPlaybackSamplesToRemove = static_cast<int>(static_cast<float>(checkEntry.nPlaybackSamples) * proportionOfAvailableSamplesToRemove);
        auto newNPlaybackSamples = checkEntry.nPlaybackSamples - numberOfPlaybackSamplesToRemove;
        auto newPlaybackSamplesAtStart = checkEntry.playbackSamplesAtStart + numberOfPlaybackSamplesToRemove;
        auto newPlaybackSamplesAtEnd = checkEntry.playbackSamplesAtEnd;
        checkEntry.setPlaybackSamples(newPlaybackSamplesAtStart, newPlaybackSamplesAtEnd, newNPlaybackSamples);
        checkEntry.calculateTimeRatio();
        jassert(checkEntry.nPlaybackSamples == checkEntry.nPlaybackSamplesCalculated);
        if (checkEntry.nAvailableSamples <= 0)
        {
            jassert(checkEntry.nPlaybackSamples == 0);
            addPlayedEntry(checkEntry);
            availableStretchedEntries.erase(availableStretchedEntries.begin());
        }
        samplesToRemove -= removableSamples;
    }
    if (samplesToRemove > 0)
    {
        removeSamplesFromLog(samplesToRemove);
    }
}