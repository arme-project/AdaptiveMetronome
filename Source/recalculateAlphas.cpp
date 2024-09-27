// Include files
#include "recalculateAlphas.h"
#include "getAlphas.h"

// Function Declarations
static boolean_T argInit_boolean_T_true();
static boolean_T argInit_boolean_T_false();

static int argInit_d20x1_real_T(double result_data[], int seed);

static double argInit_real_T(double input);

// Function Definitions
static boolean_T argInit_boolean_T_true()
{
    return 1;
}

static boolean_T argInit_boolean_T_false()
{
    return false;
}

static int argInit_d20x1_real_T(double result_data[], int seed)
{

    juce::Random randomizer = juce::Random(seed);
    
    int result_size;
    // Set the size of the array.
    // Change this size to the value that the application requires.
    result_size = 12;
    result_data[0] = argInit_real_T(0.0);

    // Loop over the array to initialize each element.
    for (int idx0{ 1 }; idx0 < result_size; idx0++) {
        float input = ((randomizer.nextFloat() * 50.0) - 25.0) + (idx0*500.0);
        // Set the value of the array element.
        // Change this value to the value that the application requires.
        result_data[idx0] = argInit_real_T(input);
    }
    return result_size;
}

static double argInit_real_T(double input)
{
    return input;
}

//int getAlphasCppMain(int, char**)
//{
//    // The initialize function is being called automatically from your entry-point
//    // function. So, a call to initialize is not included here. Invoke the
//    // entry-point functions.
//    // You can call entry-point functions multiple times.
//    getAlphasCpp();
//    // Terminate the application.
//    // You do not need to do this more than one time.
//    getAlphas_terminate();
//    return 0;
//}

//double getAlphasCppTest()
//{
//    const int nOnsets = 12;
//    double vl1_data[nOnsets];
//    double vl2_data[nOnsets];
//    double vla_data[nOnsets];
//    double vlc_data[nOnsets];
//    double alphaE[4][4];
//    double smE[4];
//    double stE[4];
//    int vl1_size;
//    int vl2_size;
//    int vla_size;
//    int vlc_size;
//    // Initialize function 'getAlphas' input arguments.
//    // Initialize function input argument 'vl1'.
//    vl1_size = argInit_d20x1_real_T(vl1_data, 1);
//    // Initialize function input argument 'vl2'.
//    vl2_size = argInit_d20x1_real_T(vl2_data, 2);
//    // Initialize function input argument 'vla'.
//    vla_size = argInit_d20x1_real_T(vla_data, 3);
//    // Initialize function input argument 'vlc'.
//    vlc_size = argInit_d20x1_real_T(vlc_data, 4);
//    // Call the entry-point 'getAlphas'.
//    getAlphas(vl1_data, &vl1_size, vl2_data, &vl2_size, vla_data, &vla_size,
//        vlc_data, &vlc_size, 1, alphaE, stE, smE);
//
//    return alphaE[1][2];
//}

static std::vector<double> get48x1Double(int seed)
{

    juce::Random randomizer = juce::Random(seed);

    int result_size;
    // Set the size of the array.
    // Change this size to the value that the application requires.
    std::vector<double> result_data;
    result_size = 48;
    result_data.push_back(0.0);

    // Loop over the array to initialize each element.
    for (int idx0{ 1 }; idx0 < result_size; idx0++) {
        float input = ((randomizer.nextFloat() * 50.0) - 25.0) + (idx0 * 500.0);
        // Set the value of the array element.
        // Change this value to the value that the application requires.
        result_data.push_back((double)input);
    }
    return result_data;
}

std::vector<std::vector<double>> getAlphasCpp(std::deque<double> vl1_dq, std::deque<double> vl2_dq, std::deque<double> vla_dq, std::deque<double> vlc_dq) {

#ifdef RANDOM_ALPHAS
    std::vector<double> vl1_data = get48x1Double(0);
    std::vector<double> vl2_data = get48x1Double(1);
    std::vector<double> vla_data = get48x1Double(2);
    std::vector<double> vlc_data = get48x1Double(3);

#else
    std::vector<double> vl1_data = { vl1_dq.begin(),vl1_dq.end() };
    std::vector<double> vl2_data = { vl2_dq.begin(),vl2_dq.end() };
    std::vector<double> vla_data = { vla_dq.begin(),vla_dq.end() };
    std::vector<double> vlc_data = { vlc_dq.begin(),vlc_dq.end() };
#endif // RANDOM_ALPHAS

    int vlx_size = vl1_data.size();

    double alphaE[4][4];
    double smE[4];
    double stE[4];

    std::vector<std::vector<double> > alphaE_vector(4,std::vector<double>(4));

    getAlphas(&vl1_data[0], &vl2_data[0], &vla_data[0], 
        &vlc_data[0], alphaE, stE, smE);

    int counter = 0;
    for (int n = 0; n < 4; n++) {
        for (int m = 0; m < 4; m++) {
            alphaE_vector[m][n] = alphaE[n][m];
            counter++;
        }
    }

    return alphaE_vector;
}


// End of code generation (main.cpp)
