/*
  ==============================================================================

    TimingModelParametersGroup.h
    Created: 2 Sep 2024 12:12:47pm
    Author:  genia

  ==============================================================================
*/
#pragma once
#include <vector>
#include <string>
#include "TimingModelParameter.h"

namespace ARMETimingModel
{
#pragma region Base Model Parameters
    class ModelParameters
    {
    public:
        ModelParameters(int numberOfPlayers = 4);
        ~ModelParameters() {};

        // Add/Register Parameter names
        virtual void addFloatParameter(std::string paramName, afloat* floatValue) {};
        virtual void addRandomFloatParameter(std::string paramName, afloat* floatValue) {};
        virtual float getFloatParameterByName(std::string paramName) { return -999.0f; };
        //virtual void addFloatVectorParameter(std::string parameterName) {};
        //virtual void addFloatMatrixParameter(std::string parameterName) {};
        //virtual float getFloatParameterByNameAndPlayerIndex(std::string paramName, int playerIndex) { return 0.0f;};
        //virtual float getMatrixParameterByNameAndPlayerIndices(std::string paramName, int player1Index, int player2Index) { return 0.0f;};

        std::atomic<float> testParameter = 1.3f;
        std::atomic<float> testSigma = 1.5f;

        // Getters/Setters
        int getNumberOfPlayers() { return numberOfPlayers; }
    public:
        std::vector<std::string> listOfParameterNames;
        FloatVector listOfFloatParameters;
        std::vector<FloatVector*> listOfFloatParameterVectors;
        std::vector<FloatMatrix*> listOfFloatParameterMatrices;

        int numberOfPlayers;
    };
#pragma endregion

#pragma region Standard Model Parameters
    class StdModelParameters : public ModelParameters
    {
    public:
        StdModelParameters(int numberOfPlayers = 4);
        ~StdModelParameters() {}

        void addFloatParameter(std::string paramName, afloat* floatValue = new afloat(0.0f)) override;
        void addFloatParameter(std::string paramName, std::function<float()> getFloatFcn);
        void addRandomFloatParameter(std::string paramName, afloat* floatValue = new afloat(0.0f)) override;

        void overrideFloatParameterValue(std::string paramName, afloat* newFloatRef);
        float getFloatParameterByName(std::string paramName) override;
        void setFloatParameterByName(std::string paramName, float newFloatValue);
        //float getFloatParameterByNameAndPlayerIndex(std::string paramName, int playerIndex) override;
        //float getMatrixParameterByNameAndPlayerIndices(std::string paramName, int player1Index, int player2Index) override;
    private:
        int getIndexOfFloatParamByName(std::string paramName);
    };
#pragma endregion
}
