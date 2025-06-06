/*
  ==============================================================================

    OSCHandler.h
    Created: 6 Jun 2025 11:39:45am
    Author:  jhund

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class OSCHandler  : public juce::Component
{
public:
    OSCHandler();
    ~OSCHandler() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSCHandler)
};
