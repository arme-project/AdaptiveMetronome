#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AdaptiveMetronomeAudioProcessorEditor::AdaptiveMetronomeAudioProcessorEditor (AdaptiveMetronomeAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      loadMidiButton ("Load MIDI")
{
    addAndMakeVisible (loadMidiButton);
    loadMidiButton.changeWidthToFitText (50);
    
    loadMidiButton.addListener (this);
    
    setSize (400, 300);
}

AdaptiveMetronomeAudioProcessorEditor::~AdaptiveMetronomeAudioProcessorEditor()
{
}

//==============================================================================
void AdaptiveMetronomeAudioProcessorEditor::paint (juce::Graphics& g)
{
}

void AdaptiveMetronomeAudioProcessorEditor::resized()
{
    loadMidiButton.setTopLeftPosition (0, 0);
}

//==============================================================================
void AdaptiveMetronomeAudioProcessorEditor::buttonClicked (juce::Button *button)
{
    loadMidiFile();
}

//==============================================================================
void AdaptiveMetronomeAudioProcessorEditor::loadMidiFile()
{
    fileChooser = std::make_unique <juce::FileChooser> ("Load MIDI",
                                                        juce::File(),
                                                        "*.mid");
                                                         
    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    
    fileChooser->launchAsync (flags,
                              [this] (const juce::FileChooser& chooser)
                              {
                                  processor.loadMidiFile (chooser.getResult());
                              });
}