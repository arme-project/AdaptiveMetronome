/*
  ==============================================================================

    TimingModelParametersGroup.cpp
    Created: 2 Sep 2024 12:12:47pm
    Author:  genia

  ==============================================================================
*/

#include "TimingModelParametersGroup.h"
namespace ARMETimingModel
{
    PhaseCorrectionModelParameters::PhaseCorrectionModelParameters(int numberOfPlayers) :
        ModelParameters(numberOfPlayers),
        mNoiseStdParameter(*this, "mNoise"),
        tkNoiseStdParameter(*this, "tkNoise"),
        alphaParameter(*this, "alpha")
    {

        addFloatVectorParameter("mNoise", numberOfPlayers, 15.0f, true);
        addFloatVectorParameter("tkoise", numberOfPlayers, 5.0f, true);
        addFloatMatrixParameter("alpha", numberOfPlayers, numberOfPlayers, 0.1f);

        //const std::string prefix("player");

        //for (size_t i = 0; i < numberOfPlayers; i++)
        //{
        //    std::string pi(std::to_string(i));
        //    addFloatParameter(prefix + pi + "-mnoise-std");
        //    addFloatParameter(prefix + pi + "-tknoise-std");

        //    for (size_t j = 0; j < numberOfPlayers; j++)
        //    {
        //        std::string pj(std::to_string(i));
        //        addFloatParameter("alpha-" + pi + "-" + pj);
        //        addFloatParameter("beta-" + pi + "-" + pj);
        //    }
        //}
    }

    PhaseCorrectionModelParameters::~PhaseCorrectionModelParameters()
    {

    }

    // ADD PARAMETERS
    void PhaseCorrectionModelParameters::addFloatParameter(std::string paramName, a_float* floatValue)
    {
        //listOfFloatParameterNames.push_back(paramName);
        //auto newFloatParameter = new ModelParameterFloat(floatValue);
        //listOfFloatParameters.push_back(newFloatParameter);
        dictOfFloatParams.insert_or_assign(paramName, new ModelParameterFloat(floatValue));
    }

    void PhaseCorrectionModelParameters::addFloatParameter(std::string paramName, std::function<float()> getFloatFcn)
    {
        //listOfFloatParameterNames.push_back(paramName);
        //auto newFloatParameter = new ModelParameterFloat(getFloatFcn);
        //listOfFloatParameters.push_back(newFloatParameter);
        dictOfFloatParams.insert_or_assign(paramName, new ModelParameterFloat(getFloatFcn));

    }

    void PhaseCorrectionModelParameters::addRandomFloatParameter(std::string paramName, a_float* sigmaValue)
    {
        //listOfFloatParameterNames.push_back(paramName);
        //auto newFloatParameter = new ModelParameterRandomNormalFloat(sigmaValue);
        //listOfFloatParameters.push_back(newFloatParameter);
        dictOfFloatParams.insert_or_assign(paramName, new ModelParameterRandomNormalFloat(sigmaValue));
    }

    void PhaseCorrectionModelParameters::addFloatVectorParameter(std::string paramName, size_t size, float initValue, bool isRandomParam)
    {
        //listOfFloatVectorNames.push_back(paramName);

        auto newFloatVector = new ModelFloatVector();

        for (int i_vector = 0; i_vector < size; i_vector++)
        {
            //newFloatVector->push_back(new ModelParameterFloat(initValue));
            if (isRandomParam)
            {
                newFloatVector->push_back(new ModelParameterRandomNormalFloat((float)i_vector));
            }
            else
            {
                newFloatVector->push_back(new ModelParameterFloat((float)i_vector));
            }
        }

        //listOfFloatVectors.push_back(newFloatVector);
        dictOfVectorParams.insert_or_assign(paramName, newFloatVector);
    }

    void PhaseCorrectionModelParameters::addFloatMatrixParameter(std::string paramName, size_t size_a, size_t size_b, float initValue, bool isRandomParam)
    {
        auto newMatrix = new ModelFloatMatrix();

        for (auto i_vector1 = 0; i_vector1 < size_a; i_vector1++)
        {
            auto newFloatVector = new ModelFloatVector();

            for (int i_vector2 = 0; i_vector2 < size_b; i_vector2++)
            {
                if (isRandomParam)
                {
                    newFloatVector->push_back(new ModelParameterRandomNormalFloat((float)i_vector2));
                }
                else
                {
                    newFloatVector->push_back(new ModelParameterFloat((float)i_vector2));
                }
            }
            newMatrix->push_back(*newFloatVector);
        }
        dictOfMatrixParams.insert_or_assign(paramName, newMatrix);
    }

 
    // GET PARAMETER VALUES
    float PhaseCorrectionModelParameters::getFloatValueByName(std::string paramName)
    {
            ModelFloatPtr floatParam = *(getFloatRefByName(paramName));

            if (floatParam != nullptr)
            {
                return floatParam->getRawValue();
            }
            else
            {
                return -990.0f;
            }
    }

    float PhaseCorrectionModelParameters::getFloatValueByName(std::string paramName, size_t indexOfValue)
    {
        ModelFloatVector* floatVector = getVectorRefByName(paramName);

        if (floatVector != nullptr)
        {
            if (indexOfValue < floatVector->size()) {
                ModelParameterFloat* floatParameter = (*floatVector)[indexOfValue];
                return floatParameter->getRawValue();
            }
            else
            {
                return -992.0f;
            }
        }
        else
        {
            return -990.0f;
        }
    }

    float PhaseCorrectionModelParameters::getFloatValueByName(std::string paramName, size_t indexOfValue1, size_t indexOfValue2)
    {
        ModelFloatMatrix* floatMatrixPtr = getMatrixRefByName(paramName);

        if (floatMatrixPtr != nullptr)
        {
            ModelFloatMatrix floatMatrix = *floatMatrixPtr;
            if (indexOfValue1 < floatMatrixPtr->size())
            {
                auto floatVectorPtr = &(floatMatrixPtr->at(indexOfValue1));

                if (floatVectorPtr != nullptr)
                {
                    if (indexOfValue2 < floatVectorPtr->size()) {
                        ModelFloatPtr floatParameterPtr = floatVectorPtr->at(indexOfValue2);
                        return floatParameterPtr->getRawValue();
                    }
                    else
                    {
                        return -992.0f; // Index2 out of bounds
                    }
                }
                else
                {
                    return -990.0f; // Float vector is nullptr
                }
            }
            else
            {
                return -993.0f; // Index1 out of bounds
            }
        }
        else
        {
            return -994.0f; // Float matrix is nullptr
        }
    }

    // GET PARAMETER REFS
    ModelFloatPtr* PhaseCorrectionModelParameters::getFloatRefByName(std::string paramName)
    {
        //auto index = getIndexOfFloatParamByName(paramName);

        //if (index != -1 && index <= listOfFloatParameters.size())
        //{
        //    return &listOfFloatParameters[index];
        //}
        //else
        //{
        //    return nullptr;
        //}
        ModelFloatPtr ptrToFloatParam = findInMap<ModelFloatPtr>(paramName, dictOfFloatParams);
        return &ptrToFloatParam;
    }
    
    ModelFloatVector* PhaseCorrectionModelParameters::getVectorRefByName(std::string paramName)
    {
        //auto indexOfVector = getIndexOfFloatVectorByName(paramName);

        //if (indexOfVector != -1 && indexOfVector <= listOfFloatVectors.size())
        //{
        //    return listOfFloatVectors[indexOfVector];
        //}
        //else {
        //    return nullptr;
        //}

        return findInMap<ModelFloatVector*>(paramName, dictOfVectorParams);
    }

    ModelFloatMatrix* PhaseCorrectionModelParameters::getMatrixRefByName(std::string paramName)
    {
        return findInMap<ModelFloatMatrix*>(paramName, dictOfMatrixParams);
    }

    void PhaseCorrectionModelParameters::overrideFloatParameterByName(std::string paramName, std::function<float()> newGetFloatFcn)
    {
        overrideModelParameterFloat(getFloatRefByName(paramName), newGetFloatFcn);
    }

    void PhaseCorrectionModelParameters::overrideFloatParameterByName(std::string paramName, size_t indexOfValue, std::function<float()> newGetFloatFcn)
    {
        ModelFloatVector* modelFloatVectorPtr = getVectorRefByName(paramName);

        if (modelFloatVectorPtr != nullptr)
        {
            if (indexOfValue < modelFloatVectorPtr->size())
            {
                // Deref the pointer to the vector. 
                // Get the ModelFloatPtr at the correct index.
                // Send Ptr to this ModelFloatPtr to the override function. Has to be a Ptr to a Ptr. 
                overrideModelParameterFloat(&((*modelFloatVectorPtr)[indexOfValue]), newGetFloatFcn);
            }
        }
    }

    // OVERRIDE PARAMETER
    void PhaseCorrectionModelParameters::overrideModelParameterFloat(ModelFloatPtr* ptrToModelFloatPtr, std::function<float()> newGetFloatFcn)
    {
        // Delete the underlying ModelParameterFloat object that is pointer to by the ModelFloatPtr
        delete *ptrToModelFloatPtr;
        // Create a new MPF to be referenced 
        *ptrToModelFloatPtr = new ModelParameterFloat(newGetFloatFcn);
    }

    // SET PARAMETERS
    void PhaseCorrectionModelParameters::setFloatParameterByName(std::string paramName, float newFloatValue)
    {

        //int index = getIndexOfFloatParamByName(paramName);

        //if (index != -1 && index <= listOfFloatParameters.size())
        //{
        //    auto floatParam = listOfFloatParameters[index];

        //    if (floatParam != nullptr)
        //    {
        //        floatParam->setNewValue(newFloatValue);
        //    }
        //}
        (*getFloatRefByName(paramName))->setNewValue(newFloatValue);
    }


    //// GET INDEXES SPECIFIC
    template <typename ParamType>
    ParamType PhaseCorrectionModelParameters::findInMap(std::string paramName, std::map<std::string, ParamType> searchMap)
    {
        if (auto searchItr = searchMap.find(paramName); searchItr != searchMap.end())
        {
            return searchItr->second;
        }
        else
        {
            return nullptr;
        }
    }
    //size_t PhaseCorrectionModelParameters::getIndexOfFloatParamByName(std::string paramName)
    //{
    //    return getIndexOfParamByName(&listOfFloatParameterNames, &paramName);
    //}

    //size_t PhaseCorrectionModelParameters::getIndexOfFloatVectorByName(std::string paramName)
    //{
    //    return getIndexOfParamByName(&listOfFloatVectorNames, &paramName);
    //}

    //size_t PhaseCorrectionModelParameters::getIndexOfFloatMatrixByName(std::string paramName)
    //{
    //    return getIndexOfParamByName(&listOfFloatMatrixNames, &paramName);
    //}

    //// GET INDEX GENERIC
    //size_t PhaseCorrectionModelParameters::getIndexOfParamByName(std::vector<std::string> *listOfParamNames, std::string* paramName)
    //{
    //    auto it = find(listOfParamNames->begin(), listOfParamNames->end(), *paramName);

    //    if (it != listOfParamNames->end())
    //    {
    //        // calculating the index 
    //        // of K 
    //        size_t index = it - listOfParamNames->begin();
    //        return index;
    //    }
    //    else {
    //        // If the element is not 
    //        // present in the vector 
    //        return -1;
    //    }
    //}

    //ModelParameterFloatRef::ModelParameterFloatRef(std::function<float()> getValueFcn) :
    //    getValueFcn(getValueFcn)
    //{
    //}
}