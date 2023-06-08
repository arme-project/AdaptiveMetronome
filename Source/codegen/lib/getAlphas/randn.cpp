//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// randn.cpp
//
// Code generation for function 'randn'
//

// Include files
#include "randn.h"
#include "eml_rand_mt19937ar.h"
#include "getAlphas_data.h"
#include "rt_nonfinite.h"

// Function Definitions
//
//
namespace coder {
void b_randn(double r[4][20])
{
  for (int k{0}; k < 80; k++) {
    (&r[0][0])[k] = eml_rand_mt19937ar(state);
  }
}

//
//
void randn(double r[4][19])
{
  for (int k{0}; k < 76; k++) {
    (&r[0][0])[k] = eml_rand_mt19937ar(state);
  }
}

} // namespace coder

// End of code generation (randn.cpp)
