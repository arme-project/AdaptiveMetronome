/*
  ==============================================================================

    TimingModel.h
    Created: 22 Aug 2024 2:07:50pm
    Author:  Genia Penksik

  ==============================================================================
*/
#pragma once
#include "TimingModelParametersGroup.h"

namespace ARMETimingModel
{
// MODEL
#pragma region Base Timing Model
    class TimingModel
    {
    public:
        // Parameters
        TimingModel(int numberOfPlayers, int numberOfUserPlayers, float initialOnsetInterval, ModelParameters& modelParams) :
            numberOfPlayers(numberOfPlayers),
            numberOfUserPlayers(numberOfUserPlayers),
            initialOnsetInterval(initialOnsetInterval),
            modelParams(modelParams),
            nextOnsetTimes(numberOfPlayers) {}

        virtual void createNewParameters(int numberOfPlayers, int numberOfUserPlayers, float initialOnsetInterval) {};
        virtual void setModelParameters(ModelParameters& newModelParams) {};

        // Onsets
        virtual void reset() {};
        virtual void resetOnsets(int maxNumberOfOnsets) {};
        virtual void registerNewOnset(int playerNumber, float onsetTime, int onsetNumber = -1.0f) {};

        virtual float getLatestOnset(int playerNumber) { return -999.0f; };
        virtual float getOnsetForNoteNumber(int playerNumber, int noteNumber) { return -999.0f; };
        virtual float getNumberOfOnsetsRegisteredForPlayer(int playerNumber) { return -999.0f; };
        virtual float getNextOnset(int playerNumber) { return -999.0f; };
        virtual std::vector<float> getNextOnsets() { return std::vector<float>(0); };

        // Getters/Setters
        int getNumberOfPlayers() { return numberOfPlayers; }
        int getNumberOfUserPlayers() { return numberOfUserPlayers; }

    protected:
        // Basic constructor with number of players
        ~TimingModel() {};
        // Parameters
        ModelParameters& modelParams;

        // Global Parameters
        int numberOfPlayers = 4;
        int numberOfUserPlayers = 1;
        float initialOnsetInterval = 0.5f;

        int numberOfNotesRegisteredByAllPlayers = 0;
        int numberOfNextOnsetsCalculated = 0;

        // Onsets
        static const int defaultMaxNumberOfOnsets = 300;
        std::vector<std::vector<float>> onsetTimes;
        std::vector<float> nextOnsetTimes;
    };
#pragma endregion

    // 2.0 STANDARD MODEL IMPLEMENTATIONS
#pragma region Standard Phase Correction Timing Model
    /**
     * \brief Implementation of the most basic TimingModel.
     * 
     *  Implements Alpha, Timekeeper Noise and Motor Noise parameters.
     */
    class PhaseCorrectionTimingModel : public TimingModel
    {
    public:
        PhaseCorrectionTimingModel(int numberOfPlayers, int numberOfUserPlayers, float initialOnsetInterval, ModelParameters& modelParams);
        ~PhaseCorrectionTimingModel() {}

        virtual void createNewParameters(int numberOfPlayers, int numberOfUserPlayers, float initialOnsetInterval) override {};
        virtual void setModelParameters(ModelParameters& newModelParams) override { modelParams = newModelParams; };

        // Onsets
        virtual void reset();
        virtual void resetOnsets(int maxNumberOfOnsets) override;
        virtual void registerNewOnset(int playerNumber, float onsetTime, int onsetNumber = -1.0f) override;

        virtual float getLatestOnset(int playerNumber) override;
        virtual float getOnsetForNoteNumber(int playerNumber, int noteNumber) override;
        virtual float getNumberOfOnsetsRegisteredForPlayer(int playerNumber) override;
        
        virtual float getNextOnset(int playerNumber) override;
        virtual std::vector<float> getNextOnsets() override;
    };
#pragma endregion
}