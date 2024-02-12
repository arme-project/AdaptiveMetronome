#pragma once
#include <JuceHeader.h>
#include "EnsembleModel.h"

class AdaptiveMetronomeAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    AdaptiveMetronomeAudioProcessor();
    ~AdaptiveMetronomeAudioProcessor() override;

    //==============================================================================
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;

    //==============================================================================
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    
    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    
    //==============================================================================
    EnsembleModel ensemble;

    bool hasDefaultConfigBeenChecked = false;

    EnsembleModel& loadMidiFile (const juce::File &file, int userPlayers);
    EnsembleModel& loadXmlFile (const juce::File &file);
    void resetEnsemble();

private:
    //==============================================================================
    
    //==============================================================================
    juce::MidiBuffer midiOutputBuffer;
    
    //==============================================================================
    bool wasPlaying = false;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdaptiveMetronomeAudioProcessor)
};
