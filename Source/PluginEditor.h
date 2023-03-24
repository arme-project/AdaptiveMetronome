#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "MetronomeClock.h"
using namespace std::chrono;

class AdaptiveMetronomeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                              public juce::Button::Listener,
                                              private juce::OSCReceiver,
                                              private juce::OSCReceiver::ListenerWithOSCAddress <juce::OSCReceiver::MessageLoopCallback>

{
public:
    AdaptiveMetronomeAudioProcessorEditor (AdaptiveMetronomeAudioProcessor&,
                                           EnsembleModel &ensemble);
    ~AdaptiveMetronomeAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    EnsembleModel* thisEnsemble;
    //==============================================================================
    void buttonClicked (juce::Button *button) override;
    bool midiNoteReceived;
    void oscMessageReceived(const juce::OSCMessage& message) override;
    milliseconds millisecond_at_tick;
    MetronomeClock clock;

private:
    //==============================================================================
    AdaptiveMetronomeAudioProcessor &processor;
    //==============================================================================
    juce::Label instructionLabel, userPlayersLabel, midiNoteReceivedLabel;
    juce::ComboBox userPlayersSelector;
    juce::TextButton resetButton, loadMidiButton, playButton;
    std::unique_ptr <juce::FileChooser> fileChooser;
    
    //==============================================================================
    void resetButtonCallback();
    void loadMidiButtonCallback();
    void playButtonCallback();
    void loadMidiFile (juce::File file);
    void setMidiNoteReceived(bool setMidiNoteLabel);

    //==============================================================================
    class EnsembleParametersComponent : public juce::Component
    {
    public:
        EnsembleParametersComponent (EnsembleModel &ensemble);
        ~EnsembleParametersComponent();
        
        void paint (juce::Graphics &g) override;
        void resized() override;
        
        static void calculateWidthAndHeight (int nPlayers, int &width, int &height);

    private:      
        //==========================================================================
        // Labels
        std::vector <std::unique_ptr <juce::Label> > headingLabels;
        std::vector <std::unique_ptr <juce::Label> > playerLabels;
        std::vector <std::unique_ptr <juce::Label> > alphaLabels;
        
        static const juce::StringArray headings;
        
        //==========================================================================
        // Parameter sliders and attachments
        std::vector <std::unique_ptr <juce::ComboBox> > channelSelectors;
        std::vector <std::unique_ptr <juce::Slider> > volumeSliders;
        std::vector <std::unique_ptr <juce::Slider> > delaySliders;
        std::vector <std::unique_ptr <juce::Slider> > mNoiseStdSliders;
        std::vector <std::unique_ptr <juce::Slider> > tkNoiseStdSliders;
        std::vector <std::vector <std::unique_ptr <juce::Slider> > > alphaSliders;
    
        std::vector <std::unique_ptr <juce::ComboBoxParameterAttachment> > channelAttachments;
        std::vector <std::unique_ptr <juce::SliderParameterAttachment> > volumeAttachments;
        std::vector <std::unique_ptr <juce::SliderParameterAttachment> > delayAttachments;
        std::vector <std::unique_ptr <juce::SliderParameterAttachment> > mNoiseStdAttachments;
        std::vector <std::unique_ptr <juce::SliderParameterAttachment> > tkNoiseStdAttachments;
        std::vector <std::vector <std::unique_ptr <juce::SliderParameterAttachment> > > alphaAttachments;

        //==========================================================================
        // Layout constants
        static const int headingHeight = 50;
        static const int rowHeight = 100;
        static const int columnWidth = 100;
        static const int padding = 5;
        static const int comboBoxHeight = 30;
    };
    
    juce::Viewport ensembleParametersViewport;


    static const int instructionStripHeight = 50;
    static const int optionsStripHeight = 50;
    static const int padding = 5;
    
    void initialiseEnsembleParameters (EnsembleModel &ensemble);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdaptiveMetronomeAudioProcessorEditor)
};
