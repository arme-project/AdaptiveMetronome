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
    ModelParameters::ModelParameters(int numberOfPlayers) :
        numberOfPlayers(numberOfPlayers)
    {

    }

    StdModelParameters::StdModelParameters(int numberOfPlayers) :
        ModelParameters(numberOfPlayers)
    {
        addFloatParameter("testParameter", &testParameter);
        addRandomFloatParameter("testRandomParameter", &testSigma);
    }

    void StdModelParameters::overrideFloatParameterValue(std::string paramName, a_float* newFloatRef)
    {
        int index = getIndexOfFloatParamByName(paramName);
        if (index != -1 && index <= listOfFloatParameters.size())
        {
            auto floatParam = listOfFloatParameters[index];

            if (floatParam != nullptr)
            {
                return floatParam->setNewReference(newFloatRef);
            }
        }
    }

    void StdModelParameters::addFloatParameter(std::string paramName, a_float* floatValue)
    {
        listOfParameterNames.push_back(paramName);
        auto newFloatParameter = new ModelParameterFloat(floatValue);
        listOfFloatParameters.push_back(newFloatParameter);
    }

    void StdModelParameters::addFloatParameter(std::string paramName, std::function<float()> getFloatFcn)
    {
        listOfParameterNames.push_back(paramName);
        auto newFloatParameter = new ModelParameterFloatRef(getFloatFcn);
        listOfFloatParameters.push_back(newFloatParameter);
    }

    void StdModelParameters::addRandomFloatParameter(std::string paramName, a_float* sigmaValue)
    {
        listOfParameterNames.push_back(paramName);
        auto newFloatParameter = new ModelParameterRandomNormalFloat(sigmaValue);
        listOfFloatParameters.push_back(newFloatParameter);
    }

    float StdModelParameters::getFloatParameterByName(std::string paramName)
    {
        int index = getIndexOfFloatParamByName(paramName);

        if (index != -1 && index <= listOfFloatParameters.size())
        {
            auto floatParam = listOfFloatParameters[index];

            if (floatParam != nullptr)
            {
                return floatParam->getRawValue();
            }
            else
            {
                return -990.0f;
            }
        }
        else
        {
            return -991.0f;
        }
    }

    void StdModelParameters::setFloatParameterByName(std::string paramName, float newFloatValue)
    {
        int index = getIndexOfFloatParamByName(paramName);

        if (index != -1 && index <= listOfFloatParameters.size())
        {
            auto floatParam = listOfFloatParameters[index];

            if (floatParam != nullptr)
            {
                floatParam->setNewValue(newFloatValue);
            }
        }
    }

    int StdModelParameters::getIndexOfFloatParamByName(std::string paramName)
    {
        auto it = find(listOfParameterNames.begin(), listOfParameterNames.end(), paramName);

        if (it != listOfParameterNames.end())
        {
            // calculating the index 
            // of K 
            int index = it - listOfParameterNames.begin();
            return index;
        }
        else {
            // If the element is not 
            // present in the vector 
            return -1;
        }
    }

    ModelParameterFloatRef::ModelParameterFloatRef(std::function<float()> getValueFcn) :
        getValueFcn(getValueFcn)
    {
    }
}