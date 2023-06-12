#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AdaptiveMetronomeAudioProcessorEditor::AdaptiveMetronomeAudioProcessorEditor (AdaptiveMetronomeAudioProcessor& p,
                                                                              EnsembleModel &ensemble)
    : AudioProcessorEditor (&p),
      processor (p),
      instructionLabel (juce::String(), "Wait for 4 tones, then start tapping along..."),
      userPlayersLabel (juce::String(), "No. User Players:"),
      resetButton ("Reset"),
      loadMidiButton ("Load MIDI")
{
        
    //==========================================================================
    addAndMakeVisible (instructionLabel);
    instructionLabel.setJustificationType (juce::Justification::left);
    instructionLabel.setFont (instructionStripHeight - padding * 3);
    
    //==========================================================================
    addAndMakeVisible (userPlayersLabel);
    userPlayersLabel.setJustificationType (juce::Justification::right);
    userPlayersLabel.setFont (optionsStripHeight - padding * 5);
    
    addAndMakeVisible (userPlayersSelector);
    
    for (int s = 0; s <= 4; ++s)
    {
        userPlayersSelector.addItem (juce::String (s), s + 1);
    }
    
    userPlayersSelector.setSelectedId (2);
    
    //==========================================================================
    addAndMakeVisible (resetButton);
    resetButton.addListener (this);
    
    addAndMakeVisible (loadMidiButton);    
    loadMidiButton.addListener (this);
    
    addAndMakeVisible (ensembleParametersViewport);
    
    //==========================================================================
    initialiseEnsembleParameters (ensemble);
    
    //==========================================================================
    int paramWidth = 0, paramHeight = 0;
    EnsembleParametersComponent::calculateWidthAndHeight (4, paramWidth, paramHeight);
    
    setSize (paramWidth, paramHeight + instructionStripHeight + optionsStripHeight);
}

AdaptiveMetronomeAudioProcessorEditor::~AdaptiveMetronomeAudioProcessorEditor()
{
}

//==============================================================================
void AdaptiveMetronomeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void AdaptiveMetronomeAudioProcessorEditor::resized()
{
    //==========================================================================
    auto bounds = getLocalBounds();
    
    //==========================================================================
    // Static strip at top of screen.
    instructionLabel.setBounds (bounds.removeFromTop (instructionStripHeight).reduced (padding));
    
    //==========================================================================
    // Static strip at bottom of screen.
    auto optionsStripBounds = bounds.removeFromBottom (optionsStripHeight);
    
    auto loadButtonBounds = optionsStripBounds.removeFromRight (loadMidiButton.getBestWidthForHeight (optionsStripHeight));
    loadMidiButton.setBounds (loadButtonBounds.reduced (padding));
    
    auto resetButtonBounds = optionsStripBounds.removeFromRight (resetButton.getBestWidthForHeight (optionsStripHeight));
    resetButton.setBounds (resetButtonBounds.reduced (padding));
    
    auto userPlayersBounds = optionsStripBounds.removeFromRight (100);
    userPlayersSelector.setBounds (userPlayersBounds.reduced (padding));
    
    userPlayersLabel.setBounds (optionsStripBounds);
    
    //==========================================================================
    // Ensemble Parameters Area
    ensembleParametersViewport.setBounds (bounds);
}

//==============================================================================
void AdaptiveMetronomeAudioProcessorEditor::buttonClicked (juce::Button *button)
{
    if (button == &resetButton)
    {
        resetButtonCallback();
    }
    else
    {
        loadMidiButtonCallback();
    }
}

//==============================================================================
void AdaptiveMetronomeAudioProcessorEditor::resetButtonCallback()
{
    processor.resetEnsemble();
}

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
    
    auto &ensemble = processor.loadMidiFile (file, userPlayersSelector.getSelectedId() - 1);
    initialiseEnsembleParameters (ensemble);
}

//==============================================================================
const juce::StringArray AdaptiveMetronomeAudioProcessorEditor::EnsembleParametersComponent::headings {"Player",
                                                                                                      "MIDI Channel",
                                                                                                      "Volume",
                                                                                                      "Delay",
                                                                                                      "Motor Noise STD",
                                                                                                      "Time Keeper Noise STD",
                                                                                                      "Alphas"};

AdaptiveMetronomeAudioProcessorEditor::EnsembleParametersComponent::EnsembleParametersComponent (EnsembleModel &ensemble)
{
    //==========================================================================
    // Heading Labels
    for (int i = 0; i < headings.size(); ++i)
    {
        headingLabels.push_back (std::make_unique <juce::Label> (juce::String(), headings [i]));
        headingLabels [i]->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (*headingLabels [i]);
    }
    
    //==========================================================================
    // Player parameters
    int nPlayers = ensemble.getNumPlayers();
    
    for (int i = 0; i < nPlayers; ++i)
    {
        //======================================================================
        // Player Labels
        playerLabels.push_back (std::make_unique <juce::Label> (juce::String(), juce::String (i + 1)));
        playerLabels [i]->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (*playerLabels [i]);
        
        alphaLabels.push_back (std::make_unique <juce::Label> (juce::String(), juce::String (i + 1)));
        alphaLabels [i]->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (*alphaLabels [i]);
        
        //======================================================================
        // MIDI channel selectors
        channelSelectors.push_back (std::make_unique <juce::ComboBox> ());
        
        for (int c = 1; c <= 16; ++c)
        {
            channelSelectors [i]->addItem (juce::String (c), c);
        }
        
        addAndMakeVisible (*channelSelectors [i]);
        
        //=======================================================================
        // Parameter Sliders
        // Volume
        volumeSliders.push_back (std::make_unique <juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                                  juce::Slider::TextBoxBelow));
          
        // Delay
        delaySliders.push_back (std::make_unique <juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                                 juce::Slider::TextBoxBelow));
        delaySliders [i]->setTextValueSuffix (" ms");
        delaySliders [i]->setColour (juce::Slider::thumbColourId, juce::Colours::seagreen);                                                        
        
        // Motor Noise                                                         
        mNoiseStdSliders.push_back (std::make_unique <juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                                     juce::Slider::TextBoxBelow));
        mNoiseStdSliders [i]->setTextValueSuffix (" ms");
        mNoiseStdSliders [i]->setColour (juce::Slider::thumbColourId, juce::Colours::seagreen);
        
        // Timekeeper Noise                                                             
        tkNoiseStdSliders.push_back (std::make_unique <juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                                      juce::Slider::TextBoxBelow));
        tkNoiseStdSliders [i]->setTextValueSuffix (" ms");
        tkNoiseStdSliders [i]->setColour (juce::Slider::thumbColourId, juce::Colours::seagreen);
              
        // Add all to the UI
        addAndMakeVisible (*delaySliders [i]);
        addAndMakeVisible (*volumeSliders [i]);
        addAndMakeVisible (*mNoiseStdSliders [i]);
        addAndMakeVisible (*tkNoiseStdSliders [i]);
        
        // Hide user player parameters
        delaySliders [i]->setVisible (!ensemble.isPlayerUserOperated (i));
        mNoiseStdSliders [i]->setVisible (!ensemble.isPlayerUserOperated (i));
        tkNoiseStdSliders [i]->setVisible (!ensemble.isPlayerUserOperated (i));
                                                                     
        //=======================================================================
        // Component attachments
        channelAttachments.push_back (std::make_unique <juce::ComboBoxParameterAttachment> (ensemble.getPlayerChannelParameter (i),
                                                                                            *channelSelectors [i]));
        volumeAttachments.push_back (std::make_unique <juce::SliderParameterAttachment> (ensemble.getPlayerVolumeParameter (i),
                                                                                         *volumeSliders [i]));
        delayAttachments.push_back (std::make_unique <juce::SliderParameterAttachment> (ensemble.getPlayerDelayParameter (i),
                                                                                        *delaySliders [i]));
        mNoiseStdAttachments.push_back (std::make_unique <juce::SliderParameterAttachment> (ensemble.getPlayerMotorNoiseParameter (i),
                                                                                            *mNoiseStdSliders [i]));
        tkNoiseStdAttachments.push_back (std::make_unique <juce::SliderParameterAttachment> (ensemble.getPlayerTimeKeeperNoiseParameter (i),
                                                                                             *tkNoiseStdSliders [i]));
                                                                                            
        //=======================================================================
        // Alpha controls       
        std::vector <std::unique_ptr <juce::Slider> > alphaRow;
        std::vector <std::unique_ptr <juce::SliderParameterAttachment> > alphaAttachmentRow;

        for (int j = 0; j < nPlayers; ++j)
        {
            alphaRow.push_back (std::make_unique <juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                                 juce::Slider::TextBoxBelow));                                         
            alphaRow [j]->setColour (juce::Slider::thumbColourId, juce::Colours::indianred);
            
            alphaAttachmentRow.push_back (std::make_unique <juce::SliderParameterAttachment> (ensemble.getAlphaParameter (i, j),
                                                                                              *alphaRow [j]));
                                                                 
            addAndMakeVisible (*alphaRow [j]);
            //alphaRow [j]->setVisible (!ensemble.isPlayerUserOperated (i));
            alphaRow[j]->setVisible(true);

        }
        
        alphaSliders.push_back (std::move (alphaRow));
        alphaAttachments.push_back (std::move (alphaAttachmentRow));
    }
    
    int width = 0, height = 0;
    calculateWidthAndHeight (nPlayers, width, height);
    setSize(width, height);
}

AdaptiveMetronomeAudioProcessorEditor::EnsembleParametersComponent::~EnsembleParametersComponent()
{
}

void AdaptiveMetronomeAudioProcessorEditor::EnsembleParametersComponent::paint (juce::Graphics &g)
{
    g.fillAll (juce::Colours::black);
}

void AdaptiveMetronomeAudioProcessorEditor::EnsembleParametersComponent::resized()
{
    //==========================================================================
    // Don't place anything if there are no players
    if (alphaLabels.size() == 0)
    {
        return;
    }

    //==========================================================================
    // Place Headings
    auto bounds = getLocalBounds();
    auto headingBounds = bounds.removeFromTop (headingHeight);
    auto alphaHeadingIdx = headingLabels.size() - 1;
    
    for (int i = 0; i < alphaHeadingIdx; ++i)
    {
        headingLabels [i]->setBounds (headingBounds.removeFromLeft (columnWidth));
    }
    
    headingLabels [alphaHeadingIdx]->setBounds (headingBounds.removeFromTop (headingBounds.getHeight() / 2));
    
    for (int i = 0; i < alphaLabels.size(); ++i)
    {
        alphaLabels [i]->setBounds (headingBounds.removeFromLeft (columnWidth));
    }
    
    //==========================================================================
    // Place Sliders
    for (int i = 0; i < channelAttachments.size(); ++i)
    {
        auto rowBounds = bounds.removeFromTop (rowHeight);
        
        playerLabels [i]->setBounds (rowBounds.removeFromLeft (columnWidth));
        channelSelectors [i]->setBounds (rowBounds.removeFromLeft (columnWidth).reduced (padding, (rowHeight - comboBoxHeight) / 2)); 
        volumeSliders [i]->setBounds (rowBounds.removeFromLeft (columnWidth).reduced (padding));
        delaySliders [i]->setBounds (rowBounds.removeFromLeft (columnWidth).reduced (padding));
        mNoiseStdSliders [i]->setBounds (rowBounds.removeFromLeft (columnWidth).reduced (padding));
        tkNoiseStdSliders [i]->setBounds (rowBounds.removeFromLeft (columnWidth).reduced (padding));
        
        for (int j = 0; j < alphaSliders [i].size(); ++j)
        {
            alphaSliders [i][j]->setBounds (rowBounds.removeFromLeft (columnWidth).reduced (padding));
        }
    }
}

void AdaptiveMetronomeAudioProcessorEditor::EnsembleParametersComponent::calculateWidthAndHeight (int nPlayers, int &width, int &height)
{
    width = (6 + nPlayers) * columnWidth;
    height = rowHeight * nPlayers + headingHeight;
}

void AdaptiveMetronomeAudioProcessorEditor::initialiseEnsembleParameters (EnsembleModel &ensemble)
{
    ensembleParametersViewport.setViewedComponent (new EnsembleParametersComponent (ensemble));
}

