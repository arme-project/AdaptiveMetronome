#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "MetronomeClock.h"
#include "MatlabEngine.hpp"
#include "MatlabDataArray.hpp"

using namespace std::chrono;
class AdaptiveMetronomeAudioProcessorEditor : public juce::AudioProcessorEditor,
    public juce::Timer,
    public juce::Button::Listener,
    private juce::OSCReceiver,
    private juce::OSCReceiver::ListenerWithOSCAddress <juce::OSCReceiver::MessageLoopCallback>

{
public:
    AdaptiveMetronomeAudioProcessorEditor(AdaptiveMetronomeAudioProcessor&,
        EnsembleModel& ensemble);
    ~AdaptiveMetronomeAudioProcessorEditor() override;

    static void writeToLogger(time_point<system_clock> timeStamp, juce::String source, juce::String method, juce::String message);

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    EnsembleModel* thisEnsemble;
    //==============================================================================
    void buttonClicked(juce::Button* button) override;
    void oscMessageReceived(const juce::OSCMessage& message) override;
    milliseconds millisecond_at_tick;
    MetronomeClock clock;
    juce::File logFile = juce::File("D:/LogTest.txt");
    juce::String logString = juce::String("HELLO");
    juce::FileLogger logger = juce::FileLogger(logFile, logString, juce::int64(128 * 1024));

private:
    //==============================================================================
    AdaptiveMetronomeAudioProcessor& processor;
    //==============================================================================
    juce::Label instructionLabel, userPlayersLabel, midiNoteReceivedLabel;
    juce::ComboBox userPlayersSelector;
    juce::TextButton resetButton, loadMidiButton, playButton;
    std::unique_ptr <juce::FileChooser> fileChooser;

    //==============================================================================
    void resetButtonCallback();
    void loadMidiButtonCallback();
    void playButtonCallback();
    void loadMidiFile(juce::File file);

    //==============================================================================
    class EnsembleParametersComponent : public juce::Component
    {
    public:
        EnsembleParametersComponent(EnsembleModel& ensemble);
        ~EnsembleParametersComponent();

        void paint(juce::Graphics& g) override;
        void resized() override;

        static void calculateWidthAndHeight(int nPlayers, int& width, int& height);

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

    void initialiseEnsembleParameters(EnsembleModel& ensemble);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdaptiveMetronomeAudioProcessorEditor)
};
