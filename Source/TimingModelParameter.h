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

        ~ModelParameterFloat() {};

        /**
         * \brief Get the actual float value of this parameter.
         * \return The actual value of the underlying std::atomic<float>.
         */
        virtual float getRawValue() { return *value; }

        /**
         * Set the value of the underlying std::atomic<float> to the provided float.
         * \param newValue The new value to set for the parameter
         */
        virtual void setNewValue(float newValue) { *value = newValue; }

        /**
         * Change which underlying std::atomic<float> value is used by this parameter.
         * \param newRef Pointer to the new std::atomic<float> object to use as the underlying value of this parameter.
         */
        virtual void setNewReference(a_float* newRef) { value = newRef; }

    protected:
        ModelParameterFloat() {};
        a_float* value = new std::atomic<float>(0.0f);
    };

    /**
     * \brief A read-only child class of ModelParameterFloat where the underlying value is the float value result of a stored std::function<float()>.
     * 
     * This can be used when a float value has already been created elsewhere. The value cannot be changed through this parameter, and can only be used to
     * return the value for use in the TimingModel calculation.
     */
    class ModelParameterFloatRef : public ModelParameterFloat
    {
    public:
        /**
         * \brief Create new parameter using the provided std::function<float()> as the returned value when accessed.
         * \param getValueFcn This function must return a float value.
         */
        ModelParameterFloatRef(std::function<float()> getValueFcn);
        ~ModelParameterFloatRef() {};

        /**
         * \brief This returns the float valued result of calling the stored std::function<float()>.
         */
        float getRawValue() override { return getValueFcn(); }

        /** WARNING: THIS DOES NOTHING. THIS VERSION OF THE PARAMETER IS CURRENTLY READ ONLY */
        void setNewValue(float newValue) override { }
        /** WARNING: THIS DOES NOTHING. THIS VERSION OF THE PARAMETER IS CURRENTLY READ ONLY */
        void setNewReference(a_float* newRef) override { }
    private:
        std::function<float()> getValueFcn;
        std::function<void(float)> setValueFcn = {};
    };

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
         * \param sigma This is a pointer to a std::atomic<float> object that contains the parameter value, 
         * which corresponds to the Sigma of the underlying distribution..
         */
        ModelParameterRandomNormalFloat(a_float* sigma = new a_float(1.0f)) : ModelParameterFloat(sigma), gen(0)
        {
            updateDistribution();
        }
        ~ModelParameterRandomNormalFloat() {};

        /**
         * \brief Get a random float value for this parameter.
         * \return A randomly sampled float value from the underlying Normal Distribution.
         */
        float getRawValue() { 
            updateDistribution(); 
            return (*value > 0.0f) ? distribution(gen) : 0.0f;
        }

        /** \brief Sets the Sigma (Standard Deviation) of the underlying Normal Distribution. */
        void setNewValue(float newSigma) override { *value = newSigma; }

        /** \brief Changes the underlying pointer to the std::atomic<float> that acts as the Sigma value of the underlying distribution */
        void setNewReference(a_float* newSigmaRef) override { value = newSigmaRef; }
    private:
        void updateDistribution() 
        {
            // Do nothing if underlying value (i.e. Sigma is invalid). 
            if (*value <= 0.0f) return;
            distribution.param(std::normal_distribution<float>::param_type(*mean, *value));
        }
        a_float* mean = new std::atomic<float>(0.0f);
        std::normal_distribution<float> distribution{0.0f, 1.0f};
        std::mt19937 gen;
    };

    // TYPEDEFS
    using ModelFloatVector = std::vector<ModelParameterFloat*>;
    using ModelFloatMatrix = std::vector<ModelFloatVector>;
}