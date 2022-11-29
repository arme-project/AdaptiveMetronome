#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class AdaptiveMetronomeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                              public juce::Button::Listener
{
public:
    AdaptiveMetronomeAudioProcessorEditor (AdaptiveMetronomeAudioProcessor&,
                                           EnsembleModel &ensemble);
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
    juce::Label instructionLabel;
    juce::TextButton resetButton, loadMidiButton;
    std::unique_ptr <juce::FileChooser> fileChooser;
    
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
        std::vector <std::unique_ptr <juce::Slider> > mNoiseStdSliders;
        std::vector <std::unique_ptr <juce::Slider> > tkNoiseStdSliders;
        std::vector <std::unique_ptr <juce::Slider> > volumeSliders;
        std::vector <std::vector <std::unique_ptr <juce::Slider> > > alphaSliders;
    
        std::vector <std::unique_ptr <juce::ComboBoxParameterAttachment> > channelAttachments;
        std::vector <std::unique_ptr <juce::SliderParameterAttachment> > mNoiseStdAttachments;
        std::vector <std::unique_ptr <juce::SliderParameterAttachment> > tkNoiseStdAttachments;
        std::vector <std::unique_ptr <juce::SliderParameterAttachment> > volumeAttachments;
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
