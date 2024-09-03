/*
  ==============================================================================

    TimingModelParameter.h
    Created: 2 Sep 2024 12:13:07pm
    Author:  genia

  ==============================================================================
*/
#pragma once
#include <memory>
#include <atomic>
#include <functional>
#include <random>

namespace ARMETimingModel
{
    typedef std::atomic<float> afloat;
    
    // RANDOM GENERATOR
    //std::random_device rd;
    //std::mt19937 gen(rd());
    //std::mt19937 getRandomizer() { return gen; }

    // Model Parameters
    class ModelParameterFloat
    {
    public:
        ModelParameterFloat(afloat* initReference) { setNewReference(initReference); }
        ModelParameterFloat(float initValue) { setNewValue(initValue); }
        ~ModelParameterFloat() {};

        virtual float getRawValue() { return *value; }
        virtual void setNewValue(float newValue) { *value = newValue; }
        virtual void setNewReference(afloat* newRef) { value = newRef; }

    protected:
        ModelParameterFloat() {};
        afloat* value = new std::atomic<float>(0.0f);
    };

    class ModelParameterFloatRef : public ModelParameterFloat
    {
    public:
        ModelParameterFloatRef(std::function<float()> getValueFcn);
        ~ModelParameterFloatRef() {};
        float getRawValue() override { return getValueFcn(); }
        void setNewValue(float newValue) override { }
        void setNewReference(afloat* newRef) override { }
    private:
        std::function<float()> getValueFcn;
        std::function<void(float)> setValueFcn = {};
    };

    class ModelParameterRandomNormalFloat : public ModelParameterFloat
    {
    public:
        ModelParameterRandomNormalFloat(afloat* sigma = new afloat(1.0f)) : ModelParameterFloat(sigma), gen(0)
        {
            updateDistribution();
        }
        ~ModelParameterRandomNormalFloat() {};

        float getRawValue() { 
            updateDistribution(); 
            //std::mt19937 gen2(0);
            //float testParamValue1 = distribution(gen);
            //float testParamValue2 = distribution(gen);
            //float testParamValue3 = distribution(gen);
            return distribution(gen); 
        }

        void setNewValue(float newSigma) override { *value = newSigma; }
        void setNewReference(afloat* newSigmaRef) override { value = newSigmaRef; }
    private:
        void updateDistribution() { distribution.param(std::normal_distribution<float>::param_type(*mean, *value)); }
        afloat* mean = new std::atomic<float>(0.0f);
        std::normal_distribution<float> distribution{0.0f, 1.0f};

        std::mt19937 gen;
    };

    // TYPEDEFS
    using FloatVector = std::vector<ModelParameterFloat*>;
    using FloatMatrix = std::vector<FloatVector>;
}