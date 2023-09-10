#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "MetronomeClock.h"

using namespace juce;
using Editor = AdaptiveMetronomeAudioProcessorEditor;

//==============================================================================
AdaptiveMetronomeAudioProcessorEditor::AdaptiveMetronomeAudioProcessorEditor (AdaptiveMetronomeAudioProcessor& p,
                                                                              EnsembleModel& ensemble)
    : AudioProcessorEditor (&p),
      processor (p),
      instructionLabel (juce::String(), "Adaptive Metronome (Standalone)"),
      userPlayersLabel (juce::String(), "No. User Players:"),
      midiNoteReceivedLabel (juce::String(), "No"),
      playButton ("Play"),
      resetButton ("Reset"),
      loadMidiButton ("Load MIDI"),
      clock (MetronomeClock()),
      thisEnsemble(&ensemble)
    {
    addAndMakeVisible (instructionLabel);
    instructionLabel.setJustificationType (juce::Justification::left);
    instructionLabel.setFont (instructionStripHeight - padding * 3);
    //matlabEngine = matlab::engine::startMATLAB({ u"-desktop" });

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

    addAndMakeVisible (playButton);
    playButton.addListener (this);
    
    addAndMakeVisible(midiNoteReceivedLabel);

    addAndMakeVisible (ensembleParametersViewport);
    
    //==========================================================================
    initialiseEnsembleParameters (ensemble);
    
    //==========================================================================
    int paramWidth = 0, paramHeight = 0;
    EnsembleParametersComponent::calculateWidthAndHeight (4, paramWidth, paramHeight);
    
    setSize (paramWidth, paramHeight + instructionStripHeight + optionsStripHeight);

    //==========================================================================
    if (!connect(8080))                       // [3]
        DBG("Error: could not connect to UDP port 8080.");
    else
    {
        DBG("Connection to 8080 succeeded");
    }

    addListener(this, "/plugin");     // [4]
    addListener(this, "/oscstart");     // [4]
    addListener(this, "/tick");
    addListener(this, "/synch");
    addListener(this, "/playbackstart");
    addListener(this, "/alphas");

    startTimer(500);

    if (logFile.exists()) {
        logFile.deleteFile();
        logFile.create();
    }
    else {
        logFile.create();
    }
    juce::Logger::setCurrentLogger(&logger);
    juce::Logger::writeToLog("LOGGER TEST");
}

AdaptiveMetronomeAudioProcessorEditor::~AdaptiveMetronomeAudioProcessorEditor()
{
}

void AdaptiveMetronomeAudioProcessorEditor::writeToLogger(time_point<system_clock> timeStamp, juce::String source, juce::String method, juce::String message) {
    auto space = juce::String(" ").getLastCharacter();
    auto timeStampString = juce::String(MetronomeClock::tickToString(timeStamp));
        //.getDurationSincePlayback(timeStamp);
    juce::String formattedString;
    juce::String sourceCombined;
    sourceCombined << source << "::" << method;
    auto sourceCombinedPadded = sourceCombined.paddedRight(space, 48);
    formattedString << timeStampString 
        << " - "
        << sourceCombinedPadded
        << " --- "
        << message;
    Logger::writeToLog(formattedString);
}

//==============================================================================

//void AdaptiveMetronomeAudioProcessorEditor::oscMessageSendNewInterval(int playerNum, int noteNum, int noteTimeInMS) {
//
//    auto oscMessage = juce::OSCMessage("/newInterval");
//    if (!sender.send(oscMessage)) {
//        DBG("Error: could not send OSC message.");
//    }
//
//}



//==============================================================================
void AdaptiveMetronomeAudioProcessorEditor::oscMessageReceived(const juce::OSCMessage& message)
{
    juce::OSCAddressPattern oscPattern = message.getAddressPattern();
    juce::String oscAddress = oscPattern.toString();
    writeToLogger(MetronomeClock::tick(), "PluginEditor", "oscMessageReceived", oscAddress);

    if (oscAddress == "/plugin") {
        if (message[0].isFloat32() && message[1].isInt32() 
            && message[2].isInt32() && message[3].isFloat32()) {
            if (processor.manualPlaying) {
                // Uncomment to update tempo from Antescofo
                //thisEnsemble->setTempo((double)message[3].getFloat32());
                
                if (thisEnsemble->waitingForFirstNote && message[1].getInt32() == 0) {
                    thisEnsemble->triggerFirstNote();
                    thisEnsemble->setUserOnsetFromOsc(message[0].getFloat32(), message[1].getInt32(), message[2].getInt32());
                }
                else if (message[1].getInt32() > 0) {
                    thisEnsemble->setUserOnsetFromOsc(message[0].getFloat32(), message[1].getInt32(), message[2].getInt32());
                }
            }
        }
    }
    else if (oscAddress == "/alphas") {
        //DBG("ALPHAS RECEIVED SIZE " << message.size());
        if (message.size() == 16) {
            for (int i = 0; i < 16; i++) {
                if (message[i].isFloat32()) {
                    DBG(i << " = " << message[i].getFloat32());
                }
            }
        }
    }
    else if (oscAddress == "/playbackstart") {
        if (message[0].isInt32()) {                             // [5]
            if (thisEnsemble->waitingForFirstNote && processor.manualPlaying) {
                clock.setStartOfPlayback(message[0].getInt32());
                DBG("Start playback - " << clock.tickToString(clock.tick()));

            }
        }
    }
    else if (oscAddress == "/oscstart") {
        thisEnsemble->clock = &clock;
        thisEnsemble->reset(true);

        //processor.setManualPlaying(!processor.manualPlaying);
        processor.setManualPlaying(true);
    }
    else if (oscAddress == "/tick") {
        jassert(message[0].isInt32());

        clock.printMsMaxAsTime(message[0].getInt32());
    }
    else if (oscAddress == "/synch") {
        jassert(message[0].isInt32());
        jassert(message[1].isInt32());
        jassert(message[2].isInt32());
        jassert(message[3].isInt32());
        auto expectedHours = message[1].getInt32();
        auto expectedMinutes = message[2].getInt32();
        auto expectedSeconds = message[3].getInt32();
        auto max_clock = message[0].getInt32();
        clock.synchWithMax(expectedHours, expectedMinutes,
            expectedSeconds, 0,
            max_clock, clock.tick());
    }
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

    auto playButtonBounds = optionsStripBounds.removeFromRight(playButton.getBestWidthForHeight (optionsStripHeight));
    playButton.setBounds(playButtonBounds.reduced(padding));
    
    //auto midiNoteReceivedLabelBounds = optionsStripBounds.removeFromRight(midiNoteReceivedLabel.getBestWidthForHeight (optionsStripHeight));
    //midiNoteReceivedLabel.setBounds(midiNoteReceivedLabelBounds.reduced(padding));

    auto userPlayersBounds = optionsStripBounds.removeFromRight (100);
    userPlayersSelector.setBounds (userPlayersBounds.reduced (padding));
    
    midiNoteReceivedLabel.setBounds (optionsStripBounds);
    
    //==========================================================================
    // Ensemble Parameters Area
    ensembleParametersViewport.setBounds (bounds);
}

void AdaptiveMetronomeAudioProcessorEditor::timerCallback()
{
    if (thisEnsemble != nullptr) {
        //DBG("CHECKING NOTE INDEX");
        juce::String noteIndexString= juce::String("Player notes: ");
        for (int i = 0; i < 4; i++) {
            if (i < thisEnsemble->players.size()) {
                noteIndexString += juce::String(i);
                noteIndexString += juce::String(":");
                noteIndexString += juce::String(thisEnsemble->players[i]->getCurrentNoteIndex());
                noteIndexString += juce::String(", ");
            }
        }
        auto currentEnsembleNote = thisEnsemble->currentNoteIndex.get();
        midiNoteReceivedLabel.setText(noteIndexString, juce::NotificationType::dontSendNotification);
        
        // UNCOMMENT TO TEST ALPHA CALC
        // thisEnsemble->getAlphasFromMATLAB(true);
    }
}

//==============================================================================
void AdaptiveMetronomeAudioProcessorEditor::buttonClicked (juce::Button *button)
{
    if (button == &resetButton)
    {
        resetButtonCallback();
    }
    else if (button == &loadMidiButton)
    {
        loadMidiButtonCallback();
    }
    else {
        playButtonCallback();
        //midiNoteReceivedLabel.setText("Yes", juce::NotificationType::dontSendNotification);
    }
}

//==============================================================================
void AdaptiveMetronomeAudioProcessorEditor::playButtonCallback()
{
    processor.setManualPlaying(!processor.manualPlaying);
}

void AdaptiveMetronomeAudioProcessorEditor::resetButtonCallback()
{
    processor.setManualPlaying(false);
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
            alphaRow [j]->setRange(-1.0, 1.0, 0);
            alphaAttachmentRow.push_back (std::make_unique <juce::SliderParameterAttachment> (ensemble.getAlphaParameter (i, j),
                                                                                              *alphaRow [j]));
                                                                 
            addAndMakeVisible (*alphaRow [j]);
            alphaRow [j]->setVisible (!ensemble.isPlayerUserOperated (i));
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

