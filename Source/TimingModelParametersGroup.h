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
    /**
     * \brief Base class for a structure of ModelParameters used by a TimingModel.
     * 
     * This defines a basic interface for Registering and Accessing ModelParameters by Name and Player Index values. 
     */
    class ModelParameters
    {
    public:
        ModelParameters(int numberOfPlayers = 4);
        ~ModelParameters() {};

        // Add/Register Parameter names
        virtual void addFloatParameter(std::string paramName, a_float* floatValue) {};
        virtual void addRandomFloatParameter(std::string paramName, a_float* floatValue) {};
        virtual float getFloatParameterByName(std::string paramName) { return -999.0f; };
        //virtual void addFloatVectorParameter(std::string parameterName) {};
        //virtual void addFloatMatrixParameter(std::string parameterName) {};
        //virtual float getFloatParameterByNameAndPlayerIndex(std::string paramName, int playerIndex) { return 0.0f;};
        //virtual float getMatrixParameterByNameAndPlayerIndices(std::string paramName, int player1Index, int player2Index) { return 0.0f;};

        std::atomic<float> testParameter = 1.3f;
        std::atomic<float> testSigma = 1.5f;

        // Getters/Setters
        int getNumberOfPlayers() { return numberOfPlayers; }

    // TODO: Set this to private and replace with Getters and Setters
    public:
        // Lists of parameter names
        std::vector<std::string> listOfFloatParameterNames;
        std::vector<std::string> listOfFloatVectorNames;
        std::vector<std::string> listOfFloatMatrixNames;

        // Lists of Parameter pointers
        std::vector<ModelParameterFloat*> listOfFloatParameters;
        std::vector<ModelFloatVector*> listOfFloatParameterVectors;
        std::vector<ModelFloatMatrix*> listOfFloatParameterMatrices;

        int numberOfPlayers;
    };
#pragma endregion

#pragma region Standard Model Parameters
    /**
     * \brief Implementation of basic ModelParameters.
     * 
     * Includes Alpha, Timekeeper Noise and Motor Noise parameters. 
     */
    class StdModelParameters : public ModelParameters
    {
    public:
        /**
         * \brief Initialise a parameter group for a StdTimingModel with a set number of players.
         * \param numberOfPlayers Number of players that this model uses. 
         */
        StdModelParameters(int numberOfPlayers = 4);
        ~StdModelParameters() {}

        /**
         * \brief Register a new ModelParameterFloat for the model
         * 
         * \param paramName A name for the Parameter, used for future access.
         * \param floatValue Pointer to the underlying std::atomic<float> used by the parameter.
         */
        void addFloatParameter(std::string paramName, a_float* floatValue = new a_float(0.0f)) override;

        /**
         * \brief Register a new ModelParameterFloatRef for the model
         *
         * \param paramName A name for the Parameter, used for future access.
         * \param getFloatFcn The underlying std::function<float()> that returns the float value of this Parameter.
         */
        void addFloatParameter(std::string paramName, std::function<float()> getFloatFcn);

        /**
         * \brief Register a new ModelParameterFloatRef for the model
         *
         * \param paramName A name for the Parameter, used for future access.
         * \param floatValue Pointer to the underlying std::atomic<float> used by the parameter as it's Sigma (standard deviation)
         */
        void addRandomFloatParameter(std::string paramName, a_float* floatValue = new a_float(0.0f)) override;

        /**
         * \brief Returns the raw float value of a parameter by name.
         * 
         * \param paramName The name of the Parameter to access
         * \return The raw float value represented by this Parameter
         */
        float getFloatParameterByName(std::string paramName) override;

        /**
         * \brief Sets the raw float value of a parameter by name.
         *
         * \param paramName The name of the Parameter to change
         * \param newFloatValue The new value to assign to this Parameter
         */
        void setFloatParameterByName(std::string paramName, float newFloatValue);

        /**
         * \brief Changes the pointer to the underlying std::atomic<float> value of this parameter.
         *
         * \param paramName The name of the Parameter to change.
         * \param newFloatRef The new pointer to a std::atomic<float> value to use for this parameter.
         */
        void overrideFloatParameterValue(std::string paramName, a_float* newFloatRef);
        //float getFloatParameterByNameAndPlayerIndex(std::string paramName, int playerIndex) override;
        //float getMatrixParameterByNameAndPlayerIndices(std::string paramName, int player1Index, int player2Index) override;
    private:
        int getIndexOfFloatParamByName(std::string paramName);
    };
#pragma endregion
}
