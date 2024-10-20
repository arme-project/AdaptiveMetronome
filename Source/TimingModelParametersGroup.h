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
     * This defines a basic interface for Registering and Accessing ModelParameters by Name.
     * Initialise with the number of players (N). 
     * These parameters can be individual ModelFloatParameters, Vectors of N Parameters, or Square Matrices of NxN Parameters 
     */
    class ModelParameters
    {
    public:
        ModelParameters::ModelParameters(int numberOfPlayers) :
            numberOfPlayers(numberOfPlayers) {};
        ~ModelParameters() {};

        // Add/Register Parameter names
        virtual void addFloatParameter(std::string paramName, a_float* floatValue) {};
        virtual void addRandomFloatParameter(std::string paramName, a_float* floatValue) {};

        // Get Parameters
        virtual ModelParameterFloat** getFloatRefByName(std::string paramName) { return nullptr; };
        virtual ModelFloatVector* getVectorRefByName(std::string paramName) { return nullptr; };
        virtual ModelFloatMatrix* getMatrixRefByName(std::string paramName) { return nullptr; };
        virtual float getFloatValueByName(std::string paramName) { return -999.0f; };
        virtual float getFloatValueByName(std::string paramName, size_t indexOfValue) { return -999.0f; };
        virtual float getFloatValueByName(std::string paramName, size_t indexOfValue1, size_t indexOfValue2) { return -999.0f; };
        virtual void addFloatVectorParameter(std::string parameterName) {};
        virtual void addFloatMatrixParameter(std::string parameterName) {};

        // Getters/Setters
        int getNumberOfPlayers() { return numberOfPlayers; }

    // TODO: Set this to private and replace with Getters and Setters
    public:
        // Lists of parameter names
        int numberOfPlayers;

        // TODO Create a single parameter container parent class, and implement for Single, N Vector and NxN Matrix types. 
        // Dictionaries (std::map) of parameter name, to actual parameters. 
        // OR map to ModelParameterIndexGetter?
        std::map<std::string, ModelParameterFloat*> dictOfFloatParams;
        std::map<std::string, ModelFloatVector*> dictOfVectorParams;
        std::map<std::string, ModelFloatMatrix*> dictOfMatrixParams;


    // INDEX GETTER
        using paramType = ModelParameterFloat;

        class ModelParameterIndexGetter
        {
        protected:
            // Reference to the ModelParameters to search for parameter.
            ModelParameters& paramGroup;
            std::string paramName;
        
        public:
            ModelParameterIndexGetter(ModelParameters& paramGroup, std::string paramName) :
                paramGroup(paramGroup), paramName(paramName) {}

            ModelParameterIndexGetter(ModelParameters& paramGroup) :
                paramGroup(paramGroup) {}

            ~ModelParameterIndexGetter() {};

            virtual paramType* operator () (int i, int j)
            {
                return (paramGroup.getMatrixRefByName(paramName))->at(i).at(j);
            }
            virtual paramType* operator () (int i)
            {
                return (paramGroup.getVectorRefByName(paramName))->at(i);
            }
            virtual paramType* operator () ()
            {
                return *(paramGroup.getFloatRefByName(paramName));
            }
        };

        class ModelParameterIndexGetterFullName : public ModelParameterIndexGetter
        {
        protected:
            // String that preceedes first player index in parameter name
            //std::string before_I;
            // This now reuses the paramName variable from ModelParameterIndexGetter
            // String after first player index (and before second player index) in parameter name
            std::string after_I;
        public:
            ModelParameterIndexGetterFullName(ModelParameters& paramGroup, std::string before_i,
                std::string after_i) : ModelParameterIndexGetter(paramGroup, before_i),
                after_I(after_i)
            {
            }

            ~ModelParameterIndexGetterFullName() {}

            // Allows alphaParameter(i, j) type access to alpha and beta parameters
            virtual paramType* operator () (int i, int j) override
            {
                std::string matchString(paramName + std::to_string(i) + after_I + std::to_string(j));
                auto audioParamPtr = *(paramGroup.getFloatRefByName(matchString));
                return audioParamPtr;
            }

            // Allows volumeParameter(i) type access to all other parameters
            virtual paramType* operator ()(int i) override
            {
                std::string matchString(paramName + std::to_string(i) + after_I);
                //auto audioParamPtr = dynamic_cast <paramType> (**(paramGroup.getFloatRefByName(matchString)));
                auto audioParamPtr = *(paramGroup.getFloatRefByName(matchString));
                return audioParamPtr;
            };
        };
    };
#pragma endregion

#pragma region Standard Model Parameters
    /**
     * \brief Implementation of basic ModelParameters.
     * 
     * Includes Alpha, Timekeeper Noise and Motor Noise parameters. 
     */
    class PhaseCorrectionModelParameters : public ModelParameters
    {
    public:
        /**
         * \brief Initialise a parameter group for a StdTimingModel with a set number of players.
         * \param numberOfPlayers Number of players that this model uses. 
         */
        PhaseCorrectionModelParameters(int numberOfPlayers = 4);
        ~PhaseCorrectionModelParameters();

        // INDEX GETTERS
        ModelParameterIndexGetter mNoiseStdParameter, tkNoiseStdParameter;
        ModelParameterIndexGetter alphaParameter;

        // ADD PARAMETERS
        /**
         * \brief Register a new ModelParameterFloat for the model
         * 
         * \param paramName A name for the Parameter, used for future access.
         * \param floatValue Pointer to the underlying std::atomic<float> used by the parameter.
         */
        virtual void addFloatParameter(std::string paramName, a_float* floatValue = new a_float(0.0f)) override;

        /**
         * \brief Register a new ModelParameterFloatRef for the model
         *
         * \param paramName A name for the Parameter, used for future access.
         * \param getFloatFcn The underlying std::function<float()> that returns the float value of this Parameter.
         */
        virtual void addFloatParameter(std::string paramName, std::function<float()> getFloatFcn);

        /**
         * \brief Register a new ModelParameterFloatRef for the model
         *
         * \param paramName A name for the Parameter, used for future access.
         * \param floatValue Pointer to the underlying std::atomic<float> used by the parameter as it's Sigma (standard deviation)
         */
        virtual void addRandomFloatParameter(std::string paramName, a_float* floatValue = new a_float(0.0f)) override;

        virtual void addFloatVectorParameter(std::string paramName, size_t size, float defaultValue = 0.0f, bool isRandomParam = false);

        virtual void addFloatMatrixParameter(std::string paramName, size_t size_a, size_t size_b, float defaultValue = 0.0f, bool isRandomParam = false);

        // GET PARAMETERS
        virtual ModelParameterFloat** getFloatRefByName(std::string paramName) override;
        virtual ModelFloatVector* getVectorRefByName(std::string paramName) override;
        virtual ModelFloatMatrix* getMatrixRefByName(std::string paramName) override;

        /**
         * \brief Returns the raw float value of a parameter by name.
         * 
         * \param paramName The name of the Parameter to access
         * \return The raw float value represented by this Parameter
         */
        virtual float getFloatValueByName(std::string paramName) override;
        virtual float getFloatValueByName(std::string paramName, size_t indexOfValue) override;
        virtual float getFloatValueByName(std::string paramName, size_t indexOfValue1, size_t indexOfValue2) override;

        // SET PARAMETERS
        /**
         * \brief Sets the raw float value of a parameter by name.
         *
         * \param paramName The name of the Parameter to change
         * \param newFloatValue The new value to assign to this Parameter
         */
        virtual void setFloatParameterByName(std::string paramName, float newFloatValue);


        /**
         * \brief Changes the pointer to the underlying std::atomic<float> value of this parameter.
         *
         * \param paramName The name of the Parameter to change.
         * \param newFloatRef The new pointer to a std::atomic<float> value to use for this parameter.
         */
        virtual void overrideModelParameterFloat(ModelFloatPtr* ptrToModelFloatPtr, std::function<float()> newGetFloatFcn);
        virtual void overrideFloatParameterByName(std::string paramName, std::function<float()> newGetFloatFcn);
        virtual void overrideFloatParameterByName(std::string paramName, size_t indexOfValue, std::function<float()> newGetFloatFcn);

    protected:
        template <typename ParamType>
        ParamType findInMap(std::string paramName, std::map<std::string, ParamType> searchMap);
    };
#pragma endregion
}