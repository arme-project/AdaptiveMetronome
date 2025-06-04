#include <JuceHeader.h>
#include "ConfigHandler.h"

//==============================================================================
ConfigHandler::ConfigHandler(EnsembleModel* modelIn) :
	model(modelIn)
{
}

ConfigHandler::~ConfigHandler()
{
}

// Formats the current ensemble state to xml, and saves it to a file (currently a default file in user folder)
// Note: This currently only saves alpha and beta parameters.

/**
* \brief Saves the current configuration of the ensemble model to an XML file.
*/
void ConfigHandler::SaveConfig()
{
#ifdef JUCE_WINDOWS
	auto xmlOutput = &juce::XmlElement("EnsembleModelConfig");
	xmlOutput->setAttribute("numUserPlayers", model->getNumUserPlayers());

	auto xmlAlphas = xmlOutput->createNewChildElement("Alphas");
	auto xmlBetas = xmlOutput->createNewChildElement("Betas");
	for (int i = 0; i < model->getNumPlayers(); ++i)
	{
		for (int j = 0; j < model->getNumPlayers(); ++j)
		{
			float alpha = model->getAlphaParameter(i, j);
			float beta = model->getBetaParameter(i, j);

			juce::String xmlAlphaEntryName;
			juce::String xmlBetaEntryName;

			xmlAlphaEntryName << "Alpha_" << i << "_" << j;
			xmlBetaEntryName << "Beta_" << i << "_" << j;

			xmlAlphas->setAttribute(xmlAlphaEntryName, alpha);
			xmlBetas->setAttribute(xmlBetaEntryName, beta);
		}
	}

	auto ensembleConfigFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("EnsembleModelConfig.xml");
	xmlOutput->writeTo(ensembleConfigFile);
#endif
}

/*
* \brief Converts a.xml file to xmlElement(to be used in loadConfigFromXml)
*/
std::unique_ptr<juce::XmlElement> ConfigHandler::ParseConfigToElement(juce::File configFile)
{
	return juce::XmlDocument(configFile).getDocumentElement();
}

// loadConfigFromXml can be called directly with XmlElement ... or from a File via parseXmlConfigFileToXmlElement
/**
* Converts a given XML file into an XmlElement.
*
* \param configFile The juce::File representing the XML file to be parsed.
* \return A unique_ptr to a juce::XmlElement that represents the root element of the parsed XML document.
*/
void ConfigHandler::loadConfig(juce::File configFile)
{
	LoadConfig(ParseConfigToElement(configFile));
}


// Main method to load an XML config file
/**
* \brief Saves the current configuration of the ensemble model to an XML file.
*/
void ConfigHandler::LoadConfig(std::unique_ptr<juce::XmlElement> loadedConfig)
{
	if (loadedConfig == nullptr) { return; }

	// Flag to keep track if list of players needs to be reinitialised (e.g. number of user players has changed)
	bool playersNeedRecreating = false;
	bool ensembleNeedsResetting = false;

	// "LogSubfolder": Check if new config specifies a new subfolder to save logs to
	if (loadedConfig->hasAttribute("LogSubfolder"))
	{
		auto newLogSubfolder = loadedConfig->getStringAttribute("LogSubfolder", "");
		if (newLogSubfolder != "")
		{
			model->SetLogSubFolder(newLogSubfolder);
		}
	}

	// "LogSubfolder": Check if new config specifies a new subfolder to save logs to
	if (loadedConfig->hasAttribute("numIntroTones"))
	{
		model->SetNumIntroTones(loadedConfig->getIntAttribute("numIntroTones", 7));
	}

	// "ConfigSubfolder": Check if new config specifies new subfolder to look for config and midi files
	if (loadedConfig->hasAttribute("ConfigSubfolder"))
	{
		auto newConfigSubfolder = loadedConfig->getStringAttribute("ConfigSubfolder", "");
		if (newConfigSubfolder != "")
		{
			configSubfolder = newConfigSubfolder;
		}
	}

	// "LogFilename": Check if log filename should be overriden from default
	if (loadedConfig->hasAttribute("LogFilename"))
	{
		auto newLogFilename = loadedConfig->getStringAttribute("LogFilename", "");
		if (newLogFilename != "")
		{
			if (!newLogFilename.endsWith(".csv")) {
				newLogFilename << ".csv";
			}
			model->SetConfigFileNameOverride(newLogFilename);
		}
	}

	// "OSCReceivePort":
	// Check if new OSC connections requested
	if (loadedConfig->hasAttribute("OSCReceivePort"))
	{
		auto newOSCReceiverPort = loadedConfig->getIntAttribute("OSCReceivePort");
		if (newOSCReceiverPort != 0)
		{
			model->connectOSCReceiver(newOSCReceiverPort);
		}
	

	// "NumUserPlayers": Check if numUserPlayers has changed
	if (loadedConfig->hasAttribute("NumUserPlayers"))
	{
		model->SetNumUserPlayers(loadedConfig->getIntAttribute("NumUserPlayers"));
		playersNeedRecreating = true;
	}

	// "MidiFilename": Check if new midi file has been specified in config, and load it.
	if (loadedConfig->hasAttribute("MidiFilename"))
	{
		auto midiFilename = loadedConfig->getStringAttribute("MidiFilename");
		auto midiFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(configSubfolder).getChildFile(midiFilename);

		if (!midiFile.existsAsFile()) { return; }

		model->loadMidiFile(midiFile, model->getNumUserPlayers());

		// Players are automatically reinitialised when a new midi file is loaded, so flag can be set back to false
		playersNeedRecreating = false;
	}

	// Limit number of user players to the number of available tracks in the loaded midi file
	juce::MidiFile midi = model->GetMidiFile();
	int numTracks = midi.getNumTracks();
	if (model->getNumUserPlayers() > numTracks) {
		model->SetNumUserPlayers(numTracks);
	}

	if (playersNeedRecreating) {
		model->createPlayers(model->GetMidiFile());
		model->reset();
	}

	// "Alphas" and "Betas":
	auto xmlAlphas = loadedConfig->getChildByName("Alphas");
	auto xmlBetas = loadedConfig->getChildByName("Betas");

	int numPlayers = model->getNumPlayers();
	for (int i = 0; i < numPlayers; ++i)
	{
		for (int j = 0; j < numPlayers; ++j)
		{
			juce::String xmlAlphaEntryName;
			juce::String xmlBetaEntryName;

			xmlAlphaEntryName << "Alpha_" << i << "_" << j;
			xmlBetaEntryName << "Beta_" << i << "_" << j;

			// If corresponding entries are not found in xml, do not change value
			if (xmlAlphas != nullptr) {
				if (xmlAlphas->hasAttribute(xmlAlphaEntryName)) {
					model->SetAlphaParam(i, j, xmlAlphas->getDoubleAttribute(xmlAlphaEntryName));
				}
			}
			if (xmlBetas != nullptr) {
				if (xmlBetas->hasAttribute(xmlBetaEntryName)) {
					model->SetBetaParam(i, j, xmlBetas->getDoubleAttribute(xmlBetaEntryName));
				}
			}
		}
	}

	// "Motor" and "Timekeeper" noise:
	auto xmlTkNoise = loadedConfig->getChildByName("tkNoise");
	auto xmlMNoise = loadedConfig->getChildByName("mNoise");

	for (int i = 0; i < numPlayers; ++i)
	{
		juce::String xmlTkNoiseEntryName;
		juce::String xmlMNoiseEntryName;

		xmlTkNoiseEntryName << "tkNoise_" << i;
		xmlMNoiseEntryName << "mNoise_" << i;

		// If corresponding entries are not found in xml, do not change value
		if (xmlTkNoise != nullptr) {
			if (xmlTkNoise->hasAttribute(xmlTkNoiseEntryName)) {
				model->SetTimekeeperNoiseSTD(i,xmlTkNoise->getDoubleAttribute(xmlTkNoiseEntryName));
			}
		}
		if (xmlMNoise != nullptr) {
			if (xmlMNoise->hasAttribute(xmlMNoiseEntryName)) {
				model->SetMotorNoiseSTD(i, xmlMNoise->getDoubleAttribute(xmlTkNoiseEntryName));
			}
		}
	}

	model->sendActionMessage("Ensemble Reset");

	if (ensembleNeedsResetting) {
		model->reset();
	}
}