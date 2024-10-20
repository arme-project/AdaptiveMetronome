#include "XMLManager.h"

XMLManager::XMLManager(EnsembleModel* model, AdaptiveMetronomeAudioProcessor* processor)
	: ensembleModel(model), processor(processor) {}

XMLManager::~XMLManager() = default;

// Loading requires converting: xml file -> xmlDocument -> xmlElement
// loadConfigFromXml can be called directly with XmlElement 
void XMLManager::loadConfig(juce::File configFile) {

	// Parse the file into an element that we can use
	std::unique_ptr<juce::XmlElement> config = parseXmlConfigFileToXmlElement(configFile);

	if (config == nullptr) return;

	// Flag to keep track if list of players needs to be reinitialised (e.g. number of user players has changed)
	bool playersNeedRecreating = false;
	bool ensembleNeedsResetting = false;

	// Load configuration attributes
	/*if (config->hasAttribute("LogSubfolder")) {
		ensembleModel->logSubfolder = config->getStringAttribute("LogSubfolder", ensembleModel->logSubfolder);
	}
	if (config->hasAttribute("numIntroTones")) {
		ensembleModel->numIntroTones = config->getIntAttribute("numIntroTones", ensembleModel->numIntroTones);
	}
	if (config->hasAttribute("ConfigSubfolder")) {
		ensembleModel->configSubfolder = config->getStringAttribute("ConfigSubfolder", ensembleModel->configSubfolder);
	}
	if (config->hasAttribute("LogFilename")) {
		auto newLogFilename = config->getStringAttribute("LogFilename");
		if (!newLogFilename.endsWith(".csv")) {
			newLogFilename << ".csv";
		}
		ensembleModel->logFilenameOverride = newLogFilename;
	}*/

	// Wouldn't this work too? get_Attribute and hasAttribute works similarily where it checks if it has an attribute and assigns the value found (first parameter) or assign another value (second value) if not found 
	ensembleModel->logSubfolder = config->getStringAttribute("LogSubfolder", ensembleModel->logSubfolder);
	ensembleModel->numIntroTones = config->getIntAttribute("numIntroTones", ensembleModel->numIntroTones);
	ensembleModel->configSubfolder = config->getStringAttribute("ConfigSubfolder", ensembleModel->configSubfolder);

	// Log filename handling
	auto newLogFilename = config->getStringAttribute("LogFilename", ensembleModel->logFilenameOverride);
	if (!newLogFilename.isEmpty() && !newLogFilename.endsWith(".csv")) {
		newLogFilename << ".csv";
	}
	ensembleModel->logFilenameOverride = newLogFilename;

	// "OSCReceivePort":
	// Handle OSC receiver port - Check if new OSC connections requested
	if (config->hasAttribute("OSCReceivePort")) {
		int newOSCReceiverPort = config->getIntAttribute("OSCReceivePort", 0);
		if (newOSCReceiverPort != 0) {
			ensembleModel->connectOSCReceiver(newOSCReceiverPort);
		}
	}

	// "NumUserPlayers": Check if numUserPlayers has changed
	if (config->hasAttribute("NumUserPlayers"))
	{
		ensembleModel->numUserPlayers = config->getIntAttribute("NumUserPlayers");
		playersNeedRecreating = true;
	}


	// "MidiFilename": Check if new midi file has been specified in config, and load it.
	if (config->hasAttribute("MidiFilename"))
	{
		auto midiFilename = config->getStringAttribute("MidiFilename");
		auto midiFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(ensembleModel->configSubfolder).getChildFile(midiFilename);

		if (!midiFile.existsAsFile()) { return; }

		//Loads the Midifile into the ensemble model
		ensembleModel->loadMidiFile(midiFile, ensembleModel->numUserPlayers);

		// Players are automatically reinitialised when a new midi file is loaded, so flag can be set back to false
		playersNeedRecreating = false;
	}

	// Limit number of user players to the number of available tracks in the loaded midi file
	if (ensembleModel->numUserPlayers > midiFile.getNumTracks()) {
		ensembleModel->numUserPlayers = midiFile.getNumTracks();
	}

	if (playersNeedRecreating) {
		ensembleModel->createPlayers(midiFile);
		ensembleModel->reset();
	}

	// Load Alpha and Beta parameters
    auto xmlAlphas = config->getChildByName("Alphas");
    auto xmlBetas = config->getChildByName("Betas");

    for (int i = 0; i < ensembleModel->players.size(); ++i) {
        for (int j = 0; j < ensembleModel->players.size(); ++j) {
            juce::String alphaKey = "Alpha_" + juce::String(i) + "_" + juce::String(j);
            juce::String betaKey = "Beta_" + juce::String(i) + "_" + juce::String(j);

            if (xmlAlphas && xmlAlphas->hasAttribute(alphaKey)) {
                *processor->alphaParameter(i, j) = xmlAlphas->getDoubleAttribute(alphaKey);
            }
            if (xmlBetas && xmlBetas->hasAttribute(betaKey)) {
                *processor->betaParameter(i, j) = xmlBetas->getDoubleAttribute(betaKey);
            }
        }
    }

	// Load noise parameters
	auto xmlTkNoise = config->getChildByName("tkNoise");
	auto xmlMNoise = config->getChildByName("mNoise");

	for (int i = 0; i < ensembleModel->players.size(); ++i) {
		juce::String tkNoiseKey = "tkNoise_" + juce::String(i);
		juce::String mNoiseKey = "mNoise_" + juce::String(i);

		if (xmlTkNoise && xmlTkNoise->hasAttribute(tkNoiseKey)) {
			*processor->tkNoiseStdParameter(i) = xmlTkNoise->getDoubleAttribute(tkNoiseKey);
		}
		if (xmlMNoise && xmlMNoise->hasAttribute(mNoiseKey)) {
			*processor->mNoiseStdParameter(i) = xmlMNoise->getDoubleAttribute(mNoiseKey);
		}
	}

	sendActionMessage("Ensemble Reset");

	if (ensembleNeedsResetting) {
		ensembleModel->reset();
	}
}

void XMLManager::saveConfig()
{
#ifdef JUCE_WINDOWS
	auto xmlOutput = &juce::XmlElement("EnsembleModelConfig");
	xmlOutput->setAttribute("numUserPlayers", ensembleModel->numUserPlayers);

	auto xmlAlphas = xmlOutput->createNewChildElement("Alphas");
	auto xmlBetas = xmlOutput->createNewChildElement("Betas");
	for (int i = 0; i < ensembleModel->players.size(); ++i)
	{
		for (int j = 0; j < ensembleModel->players.size(); ++j)
		{
			float alpha = ensembleModel->getAlphaParameter(i, j);
			float beta = ensembleModel->getBetaParameter(i, j);

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

// Formats the current ensemble state to xml, and saves it to a file (currently a default file in user folder)
// Note: This currently only saves alpha and beta parameters.
// Takes a config file and converts it into an XML element 
std::unique_ptr<juce::XmlElement> XMLManager::parseXmlConfigFileToXmlElement(juce::File configFile) {
	return juce::XmlDocument(configFile).getDocumentElement();
}

