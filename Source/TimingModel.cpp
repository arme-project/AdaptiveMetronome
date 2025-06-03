/*
  ==============================================================================

    TimingModel.cpp
    Created: 22 Aug 2024 2:07:50pm
    Author:  Genia Penksik

  ==============================================================================
*/
#pragma once
#include "TimingModel.h"

namespace ARMETimingModel
{
    PhaseCorrectionTimingModel::PhaseCorrectionTimingModel(int numberOfPlayers, int numberOfUserPlayers, float initialOnsetInterval, ModelParameters& modelParams)
        : TimingModel(numberOfPlayers, numberOfUserPlayers, initialOnsetInterval, modelParams)
    {
        reset();
    }

    void PhaseCorrectionTimingModel::reset()
    {
        int numberOfNotesRegisteredByAllPlayers = 0;
        int numberOfNextOnsetsCalculated = 0;

        resetOnsets(300);
    }

    void PhaseCorrectionTimingModel::resetOnsets(int maxNumberOfOnsets = defaultMaxNumberOfOnsets)
    {
        for (auto playerOnsets = onsetTimes.begin(); playerOnsets != onsetTimes.end(); ++playerOnsets) 
        {
            //auto i = std::distance(onsetTimes.begin(), playerOnsets);

            playerOnsets->clear();
            playerOnsets->reserve(maxNumberOfOnsets);
        }
    }

    void PhaseCorrectionTimingModel::registerNewOnset(int playerNumber, float onsetTime, int onsetNumber)
    {
        if (onsetNumber < 0 || onsetNumber >= onsetTimes[playerNumber].size())
        {
            onsetTimes[playerNumber].push_back(onsetTime);
        }
        {
            onsetTimes[playerNumber][onsetNumber] = onsetTime;
        }
    }

    float PhaseCorrectionTimingModel::getLatestOnset(int playerNumber)
    {
        return onsetTimes[playerNumber].back();
    }

    float PhaseCorrectionTimingModel::getOnsetForNoteNumber(int playerNumber, int onsetNumber)
    {
        if (onsetNumber > onsetTimes[playerNumber].size()) return -999.0f;

        return onsetTimes[playerNumber][onsetNumber];
    }

    float PhaseCorrectionTimingModel::getNumberOfOnsetsRegisteredForPlayer(int playerNumber)
    {
        return onsetTimes[playerNumber].size();
    }

    float PhaseCorrectionTimingModel::getNextOnset(int playerNumber)
    {
        return -999.0f;
    }

    std::vector<float> PhaseCorrectionTimingModel::getNextOnsets()
    {
        // Check if needs calculating. If not, return previous values
        if (numberOfNotesRegisteredByAllPlayers <= numberOfNextOnsetsCalculated) return std::vector<float>(numberOfPlayers);
        
        return std::vector<float>();
    }
}