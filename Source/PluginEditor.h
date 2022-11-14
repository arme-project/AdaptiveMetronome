#pragma once
#include <JuceHeader.h>
#include <mutex>
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
    std::vector <std::unique_ptr <juce::Slider> > mNoiseStdSliders;
    
    std::vector <std::unique_ptr <juce::SliderParameterAttachment> > mNoiseStdAttachments;
    
    //==============================================================================
    juce::TextButton loadMidiButton;
    std::unique_ptr <juce::FileChooser> fileChooser;
    
    //==============================================================================
    void loadMidiButtonCallback();
    void loadMidiFile (juce::File file);
    
    void clearEnsembleWidgets();
    void initialiseEnsembleWidgets (EnsembleModel &ensemble);
    
    std::mutex widgetMutex;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdaptiveMetronomeAudioProcessorEditor)
};