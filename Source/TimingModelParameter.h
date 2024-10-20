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
#include <JuceHeader.h>

namespace ARMETimingModel
{
    // TODO Finalise name for this typedef
    typedef std::atomic<float> a_float;
    
    // RANDOM GENERATOR
    //std::random_device rd;
    //std::mt19937 gen(rd());
    //std::mt19937 getRandomizer() { return gen; }

    // Model Parameters
    /**
     * \brief Base class for float parameter.
     * 
     * Underlying value is a pointer to an std::atomic<float> value. The underlying pointer can be aletered, as well as the value itself. 
     */
    class ModelParameterFloat
    {
    public:
        /**
         * \brief Create new parameter using a pointer to a std::atomic<float>.
         * \param initReference This is a pointer to a std::atomic<float> object that contains the parameter value.
         */
        ModelParameterFloat(a_float* initReference) { setNewReference(initReference); }

        /**
         * \brief Create new parameter from an initial value. The underlying pointer to std::atomic<float> is auto generated.
         * \param initValue The initialisation value of the underlying std::atomic<float>.
         */
        ModelParameterFloat(float initValue) { setNewValue(initValue); }

        /**
         * \brief Create new parameter from a function that returns a float value.
         * \param getValueFcn A function that returns a float value used as the return value of this parameter.
         */
        ModelParameterFloat(std::function<float()> getValueFcn) : getValueFcn(getValueFcn) { isRefType = true;}

        ~ModelParameterFloat() { 
        };

        /**
         * \brief Get the actual float value of this parameter.
         * \return The actual value of the underlying std::atomic<float>.
         */
        virtual float getRawValue() { 
            return (isRefType) ? getValueFcn() : *value;
        }

        /**
         * Set the value of the underlying std::atomic<float> to the provided float.
         * \param newValue The new value to set for the parameter
         */
        virtual void setNewValue(float newValue) { *value = newValue; }

        /**
         * Change which underlying std::atomic<float> value is used by this parameter.
         * \param newRef Pointer to the new std::atomic<float> object to use as the underlying value of this parameter.
         */
        virtual void setNewReference(a_float* newRef) { }

        /**
         * Change this parameter to use a std::function<float()> as its return.
         * \param newGetValueFcn A function that returns a float value used as the return value of this parameter.
         */
        virtual void setNewReference(std::function<float()> newGetValueFcn)
        {
            isRefType = true;
            getValueFcn = newGetValueFcn;
        }

    protected:
        ModelParameterFloat() {};
        std::function<float()> getValueFcn;
        a_float* value = new std::atomic<float>(0.0f);
        bool isRefType = false;
    };

    ///**
    // * \brief A read-only child class of ModelParameterFloat where the underlying value is the float value result of a stored std::function<float()>.
    // * 
    // * This can be used when a float value has already been created elsewhere. The value cannot be changed through this parameter, and can only be used to
    // * return the value for use in the TimingModel calculation.
    // */
    //class ModelParameterFloatRef : public ModelParameterFloat
    //{
    //public:
    //    /**
    //     * \brief Create new parameter using the provided std::function<float()> as the returned value when accessed.
    //     * \param getValueFcn This function must return a float value.
    //     */
    //    ModelParameterFloatRef(std::function<float()> getValueFcn);
    //    ~ModelParameterFloatRef() {};

    //    /**
    //     * \brief This returns the float valued result of calling the stored std::function<float()>.
    //     */
    //    float getRawValue() override { return getValueFcn(); }

    //    /** WARNING: THIS DOES NOTHING. THIS VERSION OF THE PARAMETER IS CURRENTLY READ ONLY */
    //    void setNewValue(float newValue) override { }
    //    /** WARNING: THIS DOES NOTHING. THIS VERSION OF THE PARAMETER IS CURRENTLY READ ONLY */
    //    void setNewReference(a_float* newRef) override { }
    //private:
    //    std::function<float()> getValueFcn;
    //    //std::function<void(float)> setValueFcn = {};
    //};

    /**
     * \brief Child class of ModelParameterFloat that uses a normal distribution as the underlying value.
     * 
     * Setting the value of this parameter changes the Sigma (standard deviation) of the underlying Normal Distribution.
     * Getting the raw value returns a random sample from Normal Distribution. 
     */
    class ModelParameterRandomNormalFloat : public ModelParameterFloat
    {
    public:
        /**
         * \brief Create new normal distribution parameter using a pointer to a std::atomic<float> as the Sigma value
         * \param sigmaPtr This is a pointer to a std::atomic<float> object that contains the parameter value, 
         * which corresponds to the Sigma of the underlying distribution..
         */
        ModelParameterRandomNormalFloat(a_float* sigmaPtr = new a_float(1.0f)) : ModelParameterFloat(sigmaPtr), gen(0)
        {
            updateDistribution();
            generateNewValue();
        }
        /**
         * \brief Create new normal distribution parameter using an initial value as the Sigma value
         * \param initSigma This is a float value that will be used to create a new std::atomic<float>,
         * which corresponds to the Sigma of the underlying distribution..
         */
        ModelParameterRandomNormalFloat(float initSigma) : ModelParameterFloat(initSigma), gen(0)
        {
            updateDistribution();
            generateNewValue();
        }

        /**
         * \brief Create new normal distribution using a std::function<float()> as the Sigma value
         * \param getValueFcn This should be a std::function<float()> that returns a float value,
         * which will be used as the Sigma of the underlying distribution..
         */
        ModelParameterRandomNormalFloat(std::function<float()> getValueFcn) : ModelParameterFloat(getValueFcn), gen(0)
        {
            updateDistribution();
            generateNewValue();
        }

        ~ModelParameterRandomNormalFloat() { };

        /**
         * \brief Get the most recently generated random float.
         * \return The most recently generated float value from the underlying distribution. Use generateNewValue() to get a new value.
         */
        float getRawValue() { 
            return generateNewValue();
        }

        /**
         * \brief Generate and return a new float value.
         * \return A new float value generated from the underlying Normal Distribution
         */
        float generateNewValue() {
            updateDistribution();
            previousValue = currentValue;
            currentValue = (*value > 0.0f) ? distribution(gen) : 0.0f;
            return currentValue;
        }

        /** \brief Sets the Sigma (Standard Deviation) of the underlying Normal Distribution. */
        //void setNewValue(float newSigma) override { *value = newSigma; }

        /** \brief Changes the underlying pointer to the std::atomic<float> that acts as the Sigma value of the underlying distribution */
        //void setNewReference(a_float* newSigmaRef) override { value = newSigmaRef; }
    private:
        void updateDistribution() 
        {
            float varianceValue = (isRefType) ? getValueFcn() : *value;
            // Do nothing if underlying value (i.e. Sigma is invalid). 
            if (varianceValue <= 0.0f) return;
            distribution.param(std::normal_distribution<float>::param_type(*mean, varianceValue));
        }
        a_float* mean = new std::atomic<float>(0.0f);
        std::normal_distribution<float> distribution{0.0f, 1.0f};
        std::mt19937 gen;
        float currentValue = 0.0f;
        float previousValue = 0.0f;
    };



    // TYPEDEFS
    using ModelFloatPtr = ModelParameterFloat*;
    using ModelFloatVector = std::vector<ModelFloatPtr>;
    using ModelFloatMatrix = std::vector<ModelFloatVector>;
}