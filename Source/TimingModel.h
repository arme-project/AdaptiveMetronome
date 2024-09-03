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
        virtual void createNewParameters() {};
        virtual void setModelParameters(ModelParameters) {};

        // Onsets
        virtual void reset() {};
        virtual void resetOnsets(int maxNumberOfOnsets) {};
        virtual void registerNewOnset(int playerNumber, float onsetTime) {};

        // Getters/Setters
        int getNumberOfPlayers() { return numberOfPlayers; }
        int getNumberOfUserPlayers() { return numberOfUserPlayers; }

        virtual float getLatestOnset(int playerNumber) {};
        virtual float getOnsetForNote(int playerNumber, int noteNumber) {};
        virtual float getNextOnset(int playerNumber) {};
        virtual std::vector<float> getNextOnsets() {};

    protected:
        // Basic constructor with number of players
        TimingModel(int numberOfPlayers, int numberOfUserPlayers, float initialOnsetInterval, ModelParameters* parameters = nullptr) {};
        ~TimingModel() {};
        // Parameters
        ModelParameters* modelParams = nullptr;

        // Global Parameters
        int numberOfPlayers = 4;
        int numberOfUserPlayers = 4;
        float initialOnsetInterval = 4;

        // Onsets
        std::vector<std::vector<float>> onsetTimes;
    };
#pragma endregion

    // 2.0 STANDARD MODEL IMPLEMENTATIONS
#pragma region Standard Timing Model
    class StdTimingModel : public TimingModel
    {
    public:
        StdTimingModel(int numberOfPlayers, int numberOfUserPlayers, float initialOnsetInterval, ModelParameters* parameters = nullptr)
            : TimingModel(numberOfPlayers, numberOfUserPlayers, initialOnsetInterval) {}
        ~StdTimingModel() {}
    };
#pragma endregion
}