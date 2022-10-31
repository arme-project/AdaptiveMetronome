#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class AdaptiveMetronomeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                              public juce::Button::Listener
{
public:
    AdaptiveMetronomeAudioProcessorEditor (AdaptiveMetronomeAudioProcessor&);
    ~AdaptiveMetronomeAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    //==============================================================================
    void buttonClicked (juce::Button *button) override;

private:
    //==============================================================================
    AdaptiveMetronomeAudioProcessor &processor;
    
    //==============================================================================
    juce::TextButton loadMidiButton;
    std::unique_ptr <juce::FileChooser> fileChooser;
    
    //==============================================================================
    void loadMidiFile();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdaptiveMetronomeAudioProcessorEditor)
};