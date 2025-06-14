#pragma once

#include <JuceHeader.h>
#include "EnsembleModel.h"

class ConfigHandler  : public juce::Component
{
public:
    ConfigHandler(EnsembleModel* modelIn);
    ~ConfigHandler() override;

    void SaveConfig();
    std::unique_ptr<juce::XmlElement> ParseConfigToElement(juce::File configFile);

    void LoadConfig(std::unique_ptr<juce::XmlElement> loadedConfig);
    void LoadConfig(juce::File configFile);

	juce::String GetConfigSubfolder();
private:
    juce::String configSubfolder = "";
    EnsembleModel *model;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConfigHandler)
};
