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
    
    addAndMakeVisible (ensembleParametersViewport);
    
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
    
    //==========================================================================
    // Ensemble Parameters Area
    ensembleParametersViewport.setBounds (bounds);
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
    ensembleParametersViewport.setViewedComponent (nullptr);
    
    auto &ensemble = processor.loadMidiFile (file);
    ensembleParametersViewport.setViewedComponent (new EnsembleParametersComponent (ensemble));
}

//==============================================================================
AdaptiveMetronomeAudioProcessorEditor::EnsembleParametersComponent::EnsembleParametersComponent (EnsembleModel &ensemble)
{
    int nPlayers = ensemble.getNumPlayers();
    
    for (int i = 0; i < nPlayers; ++i)
    {
        //==============================================================================
        // MIDI channel selectors
        channelSelectors.push_back (std::make_unique <juce::ComboBox> ());
        
        for (int c = 1; c <= 16; ++c)
        {
            channelSelectors [i]->addItem (juce::String (c), c);
        }
        
        addAndMakeVisible (*channelSelectors [i]);
        
        //==============================================================================
        // Parameter Sliders
        mNoiseStdSliders.push_back (std::make_unique <juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                                     juce::Slider::TextBoxBelow));
        tkNoiseStdSliders.push_back (std::make_unique <juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                                      juce::Slider::TextBoxBelow));
        volumeSliders.push_back (std::make_unique <juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                                  juce::Slider::TextBoxBelow));
                                                                     
                                                                     
        addAndMakeVisible (*mNoiseStdSliders [i]);
        addAndMakeVisible (*tkNoiseStdSliders [i]);
        addAndMakeVisible (*volumeSliders [i]);
                                                                     
        //==============================================================================
        // Component attachments
        channelAttachments.push_back (std::make_unique <juce::ComboBoxParameterAttachment> (ensemble.getPlayerChannelParameter (i),
                                                                                            *channelSelectors [i]));
        mNoiseStdAttachments.push_back (std::make_unique <juce::SliderParameterAttachment> (ensemble.getPlayerMotorNoiseParameter (i),
                                                                                            *mNoiseStdSliders [i]));
        tkNoiseStdAttachments.push_back (std::make_unique <juce::SliderParameterAttachment> (ensemble.getPlayerTimeKeeperNoiseParameter (i),
                                                                                             *tkNoiseStdSliders [i]));
        mNoiseStdAttachments.push_back (std::make_unique <juce::SliderParameterAttachment> (ensemble.getPlayerVolumeParameter (i),
                                                                                            *volumeSliders [i]));
                                                                                            
        //==============================================================================
        // Alpha controls       
        std::vector <std::unique_ptr <juce::Slider> > alphaRow;
        std::vector <std::unique_ptr <juce::SliderParameterAttachment> > alphaAttachmentRow;

        for (int j = 0; j < nPlayers; ++j)
        {
            alphaRow.push_back (std::make_unique <juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                                 juce::Slider::TextBoxBelow));
            alphaAttachmentRow.push_back (std::make_unique <juce::SliderParameterAttachment> (ensemble.getAlphaParameter (i, j),
                                                                                              *alphaRow [j]));
                                                                 
            addAndMakeVisible (*alphaRow [j]);
        }
        
        alphaSliders.push_back (std::move (alphaRow));
        alphaAttachments.push_back (std::move (alphaAttachmentRow));
    }
    
    setSize((4 + nPlayers) * sliderWidth, rowHeight * nPlayers);
}

AdaptiveMetronomeAudioProcessorEditor::EnsembleParametersComponent::~EnsembleParametersComponent()
{
}

void AdaptiveMetronomeAudioProcessorEditor::EnsembleParametersComponent::resized()
{
    auto bounds = getLocalBounds();
    
    for (int i = 0; i < channelAttachments.size(); ++i)
    {
        auto rowBounds = bounds.removeFromTop (rowHeight);
        
        channelSelectors [i]->setBounds (rowBounds.removeFromLeft (sliderWidth));
        mNoiseStdSliders [i]->setBounds (rowBounds.removeFromLeft (sliderWidth));
        tkNoiseStdSliders [i]->setBounds (rowBounds.removeFromLeft (sliderWidth));
        volumeSliders [i]->setBounds (rowBounds.removeFromLeft (sliderWidth));
        
        for (int j = 0; j < alphaSliders [i].size(); ++j)
        {
            alphaSliders [i][j]->setBounds (rowBounds.removeFromLeft (sliderWidth));
        }
    }
}
