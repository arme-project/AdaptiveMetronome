// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>
#include <JuceHeader.h>

//#ifndef RANDOM_ALPHAS
//#define RANDOM_ALPHAS
//#endif

// Function Declarations
//extern int getAlphasCppMain(int argc, char** argv);

//juce::Random randomizer;

//double getAlphasCppTest();
std::vector<std::vector<double>> getAlphasCpp(std::deque<double> vl1_dq, std::deque<double> vl2_dq, std::deque<double> vla_dq, std::deque<double> vlc_dq);