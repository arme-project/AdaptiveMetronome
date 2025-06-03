/*
  ==============================================================================

    Poller.h
    Created: 3 Jun 2025 6:15:13pm
    Author:  jhund

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class Poller  : public juce::Component
{
public:
    Poller();
    ~Poller() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Poller)
};
