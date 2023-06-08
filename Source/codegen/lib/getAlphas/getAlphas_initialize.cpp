//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// getAlphas_initialize.cpp
//
// Code generation for function 'getAlphas_initialize'
//

// Include files
#include "getAlphas_initialize.h"
#include "eml_rand_mt19937ar_stateful.h"
#include "getAlphas_data.h"
#include "rt_nonfinite.h"

// Function Definitions
void getAlphas_initialize()
{
  eml_rand_mt19937ar_stateful_init();
  isInitialized_getAlphas = true;
}

// End of code generation (getAlphas_initialize.cpp)
