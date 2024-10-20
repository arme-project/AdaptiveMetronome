#pragma once
#include <JuceHeader.h>
#include "EnsembleModel.h"
#include "TimingModelParametersGroup.h"

using AudioParameterFloatToUse = juce::AudioParameterFloat;

class AdaptiveMetronomeAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    AdaptiveMetronomeAudioProcessor();
    void TestFloatParameters();
    ~AdaptiveMetronomeAudioProcessor() override;

    EnsembleModel ensemble;
    ARMETimingModel::PhaseCorrectionModelParameters stdModelParams;

    // Parameters stored in APVTS
    juce::AudioProcessorValueTreeState apvts;

    //==============================================================================
    //Currently maximum number of players is limited to 4
    static const int MAX_PLAYERS = 4;
    
    //==============================================================================
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;

    //==============================================================================
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    
    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    
    //==============================================================================
    bool hasDefaultConfigBeenChecked = false;

    EnsembleModel& loadMidiFile (const juce::File &file, int userPlayers);
    EnsembleModel& loadXmlFile (const juce::File &file);
    void resetEnsemble();

    // These are no longer used and will be removed
//    std::vector < std::vector < AudioParameterFloatToUse* > > alphaParameters, betaParameters;
//    std::vector < AudioParameterFloatToUse* > volumeParameters, delayParameters, mNoiseStdParameters, tkNoiseStdParameters;
//    std::vector < juce::AudioParameterInt* > channelParameters;
    
    //==============================================================================
    // Programatically creates all parameters - called by AdaptiveMetronomeAudioProcessor-->apvts constructor
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        float defaultAlpha = 0.1f;
        float defaultBeta = 0.1f;
        float defaultVolume = 1.0f;
        float defaultTkNoise = 1.0f;
        float defaultMNoise = 0.1f;
        float defaultDelay = 0.0f;
        
        juce::AudioProcessorValueTreeState::ParameterLayout params;
 
        for (int i = 0 ; i < MAX_PLAYERS ; i ++)
        {
            // Volume
            params.add(std::make_unique < AudioParameterFloatToUse > ("player" + juce::String (i) + "-volume",
                                                                       "Player " + juce::String (i) + " Volume",
                                                                       0.0, 1.0, defaultVolume));
            
            // Channel
            params.add(std::make_unique < juce::AudioParameterInt > ("player" + juce::String (i) + "-channel",
                                                                     "Player " + juce::String (i) + " MIDI Channel",
                                                                     1, 16, (i+1)));

            // Delay
            params.add(std::make_unique < AudioParameterFloatToUse > ("player" + juce::String (i) + "-delay",
                                                                       "Player " + juce::String (i) + " Delay",
                                                                       0.0, 200.0, defaultDelay));

            // Motor Noise
            params.add(std::make_unique < AudioParameterFloatToUse > ("player" + juce::String (i) + "-mnoise-std",
                                                                       "Player " + juce::String (i) + " Motor Noise Std",
                                                                       0.0, 10.0, defaultMNoise));

            // Timekeeper Noise
            params.add(std::make_unique < AudioParameterFloatToUse > ("player" + juce::String (i) + "-tknoise-std",
                                                                       "Player " + juce::String (i) + " Time Keeper Noise Std",
                                                                       0.0, 50.0, defaultTkNoise));
            
            // Inner player loop for alphas
            for (int j = 0 ; j < MAX_PLAYERS ; j++)
            {
                // Alpha
                params.add(std::make_unique < AudioParameterFloatToUse > ("alpha-" + juce::String(i) + "-" + juce::String(j),
                                                                           "Alpha " + juce::String(i) + "-" + juce::String(j),
                                                                           0.0, 1.0, defaultAlpha));

                // Beta
                params.add(std::make_unique < AudioParameterFloatToUse > ("beta-" + juce::String(i) + "-" + juce::String(j),
                                                                           "Beta " + juce::String(i) + "-" + juce::String(j),
                                                                           0.0, 1.0, defaultBeta));
            }
        }
        return params;
    }
    
    //==============================================================================
    // This is a helper class that simplifies access to parameter values.
    // Using APVTS requires searching parameters by id.
    // This allows access to parameters using bracket indexing.
    // paramType can be AudioProcessorFloat or AudioProcessorInt
    template < typename paramType >
    class ParameterIndexGetter
    {
    private:
        // Reference to the ValueTreeState to search for parameter.
        juce::AudioProcessorValueTreeState& avpts;
        // String that preceedes first player index in parameter name
        juce::String before_I;
        // String after first player index (and before second player index) in parameter name
        juce::String after_I;
    public:
        ParameterIndexGetter(juce::AudioProcessorValueTreeState& apvts_ref,
                             juce::StringRef before_i,
                             juce::StringRef after_i) :
        avpts(apvts_ref),
        before_I(juce::String(before_i)),
        after_I(juce::String(after_i))
        {
        }
        
        ~ParameterIndexGetter() {}
                
        // Allows alphaParameter(i, j) type access to alpha and beta parameters
        paramType* operator () (int i, int j)
        {
            juce::String matchString (before_I + juce::String(i) + after_I + juce::String(j));
            auto audioParamPtr = dynamic_cast < paramType* > (avpts.getParameter(juce::StringRef(matchString)));
            return audioParamPtr;
        }
        
        // Allows volumeParameter(i) type access to all other parameters
        paramType* operator ()(int i)
        {
            juce::String matchString (before_I + juce::String(i) + after_I);
            auto audioParamPtr = dynamic_cast < paramType* > (avpts.getParameter(juce::StringRef(matchString)));
            return audioParamPtr;
        };
                
    };
    
    // Initialise getters for all parameters - Defined in processor constructor
    ParameterIndexGetter < AudioParameterFloatToUse > volumeParameter, delayParameter, mNoiseStdParameter, tkNoiseStdParameter;
    ParameterIndexGetter < juce::AudioParameterInt > channelParameter;
    ParameterIndexGetter < AudioParameterFloatToUse > alphaParameter, betaParameter;
    
private:
    //==============================================================================
    juce::MidiBuffer midiOutputBuffer;
    
    //==============================================================================
    bool wasPlaying = false;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdaptiveMetronomeAudioProcessor)
};