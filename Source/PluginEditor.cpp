#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AdaptiveMetronomeAudioProcessorEditor::AdaptiveMetronomeAudioProcessorEditor (AdaptiveMetronomeAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      loadMidiButton ("Load MIDI")
{
    addAndMakeVisible (loadMidiButton);    
    loadMidiButton.addListener (this);
    
    setSize (800, 400);
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
    //==========================================================================
    auto bounds = getLocalBounds();
    
    //==========================================================================
    // Static strip at bottom of screen.
    int stripHeight = 50;
    auto stripBounds = bounds.removeFromBottom (stripHeight);
    
    int buttonPadding = 5;
    auto loadButtonBounds = stripBounds.removeFromRight (loadMidiButton.getBestWidthForHeight (stripHeight));
    loadMidiButton.setBounds (loadButtonBounds.reduced (buttonPadding));
}

//==============================================================================
void AdaptiveMetronomeAudioProcessorEditor::buttonClicked (juce::Button *button)
{
    loadMidiButtonCallback();
}

//==============================================================================
void AdaptiveMetronomeAudioProcessorEditor::loadMidiButtonCallback()
{
    fileChooser = std::make_unique <juce::FileChooser> ("Load MIDI",
                                                        juce::File(),
                                                        "*.mid");
                                                         
    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    
    fileChooser->launchAsync (flags,
                              [this] (const juce::FileChooser& chooser)
                              {
                                  loadMidiFile (chooser.getResult());
                              });
}

void AdaptiveMetronomeAudioProcessorEditor::loadMidiFile (juce::File file)
{ 
    std::unique_lock <std::mutex> lock (widgetMutex);
    
    clearEnsembleWidgets();
    
    auto &ensemble = processor.loadMidiFile (file);
    initialiseEnsembleWidgets (ensemble);
}

void AdaptiveMetronomeAudioProcessorEditor::clearEnsembleWidgets()
{
    mNoiseStdAttachments.clear();
    mNoiseStdSliders.clear();
}

void AdaptiveMetronomeAudioProcessorEditor::initialiseEnsembleWidgets (EnsembleModel &ensemble)
{
    for (int i = 0; i < ensemble.getNumPlayers(); ++i)
    {
        mNoiseStdSliders.push_back (std::make_unique <juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                                     juce::Slider::TextBoxBelow));
                                                                     
        mNoiseStdAttachments.push_back (std::make_unique <juce::SliderParameterAttachment> (ensemble.getPlayerMotorNoiseParameter (i),
                                                                                            *mNoiseStdSliders [i]));
                                                                                            
        addAndMakeVisible (*mNoiseStdSliders [i]);
    }
}