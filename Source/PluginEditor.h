#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class AdaptiveMetronomeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                              public juce::Button::Listener,
                                              public juce::ActionListener,
                                              public juce::Timer

{
public:
    AdaptiveMetronomeAudioProcessorEditor (AdaptiveMetronomeAudioProcessor&,
                                           EnsembleModel &ensemble);
    ~AdaptiveMetronomeAudioProcessorEditor() override;

    void timerCallback() override;
    void reduceAlpha();
    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void actionListenerCallback(const juce::String& message) override;

    //==============================================================================
    void buttonClicked (juce::Button *button) override;

private:
    //==============================================================================
    AdaptiveMetronomeAudioProcessor &processor;
    
    int timerInterval = 50;

    void CheckForDefaultConfig();

    //==============================================================================
    juce::Label instructionLabel, userPlayersLabel, versionLabel;
    juce::ComboBox userPlayersSelector;
    juce::TextButton resetButton, loadMidiButton;
    juce::ToggleButton oscOn;
    std::unique_ptr <juce::FileChooser> fileChooser;

    juce::TooltipWindow tooltipWindow{ this };

    //==============================================================================
    void resetButtonCallback();
    void loadMidiButtonCallback();
    void loadMidiFile (juce::File file);
    
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
        std::vector <std::unique_ptr <juce::Label> > playerRowLabels;
        std::vector <std::unique_ptr <juce::Label> > playerColumnLabels;
        std::vector <std::unique_ptr <juce::Label> > alphaLabels, betaLabels;
        
        static const juce::StringArray headings;
        
        //==========================================================================
        // Parameter sliders and attachments
        std::vector <std::unique_ptr <juce::ComboBox> > channelSelectors;
        std::vector <std::unique_ptr <juce::Slider> > volumeSliders;
        std::vector <std::unique_ptr <juce::Slider> > delaySliders;
        std::vector <std::unique_ptr <juce::Slider> > mNoiseStdSliders;
        std::vector <std::unique_ptr <juce::Slider> > tkNoiseStdSliders;
        std::vector <std::vector <std::unique_ptr <juce::Slider> > > alphaSliders;
        std::vector <std::vector <std::unique_ptr <juce::Slider> > > betaSliders;
    
        std::vector <std::unique_ptr <juce::ComboBoxParameterAttachment> > channelAttachments;
        std::vector <std::unique_ptr <juce::SliderParameterAttachment> > volumeAttachments;
        std::vector <std::unique_ptr <juce::SliderParameterAttachment> > delayAttachments;
        std::vector <std::unique_ptr <juce::SliderParameterAttachment> > mNoiseStdAttachments;
        std::vector <std::unique_ptr <juce::SliderParameterAttachment> > tkNoiseStdAttachments;
        std::vector <std::vector <std::unique_ptr <juce::SliderParameterAttachment> > > alphaAttachments;
        std::vector <std::vector <std::unique_ptr <juce::SliderParameterAttachment> > > betaAttachments;

        //==========================================================================
        // Layout constants
        static const int headingHeight = 80;
        static const int rowHeight = 100;
        static const int columnWidth = 100;
        static const int padding = 5;
        static const int comboBoxHeight = 30;
        static const int alphaBetaValWidth = columnWidth / 2;
        static const int alphaBetaValHeight = rowHeight / 7;
    };
    
    juce::Viewport ensembleParametersViewport;

    static const int instructionStripHeight = 50;
    static const int optionsStripHeight = 50;
    static const int padding = 5;
    
    void initialiseEnsembleParameters (EnsembleModel &ensemble);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdaptiveMetronomeAudioProcessorEditor)
};
