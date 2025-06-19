#pragma once
#include <JuceHeader.h>
#include "EnsembleModel.h"

/**
 * \class ConfigHandler
 * \brief Handles the configuration of the Ensemble Model.
 *
 * This class is responsible for saving and loading the configuration of the ensemble model to and from XML files.
 * It provides methods to save the current state of the model and load a previously saved configuration.
 */
class ConfigHandler : public juce::Component
{
public:
	/**
	 * \brief Constructor for ConfigHandler.
	 *
	 * \param modelIn The Ensemble Model that owns this insstance.
	 */
	ConfigHandler(EnsembleModel* modelIn);

	/**
	 * /brief Deconstructor for ConfigHandler
	 */
	~ConfigHandler() override;

	/**
	* \brief Saves the current configuration of the ensemble model to an XML file.
	*/
	void SaveConfig();

	/*
	* \brief Converts a.xml file to xmlElement(to be used in loadConfigFromXml)
	*/
	std::unique_ptr<juce::XmlElement> ParseConfigToElement(juce::File configFile);

	
	/**
	* \brief Saves the current configuration of the ensemble model to an XML file.
	*/
	void LoadConfig(std::unique_ptr<juce::XmlElement> loadedConfig);

	/**
	* \brief Converts a given XML file into an XmlElement.
	*
	* \param configFile The juce::File representing the XML file to be parsed.
	* \return A unique_ptr to a juce::XmlElement that represents the root element of the parsed XML document.
	*/
	void LoadConfig(juce::File configFile);

	/**
	* \brief Returns the configuration file name where the ensemble model's configuration is saved.
	*/
	juce::String GetConfigSubfolder();

private:
	/**
	 * \brief The subfolder where the configuration files are stored.
	 */
	juce::String configSubfolder = "";

	/**
	 * \brief The Ensemble Model that owns this instance.
	 */
	EnsembleModel* model;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfigHandler)
};
