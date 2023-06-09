//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// getAlphas.h
//
// Code generation for function 'getAlphas'
//

#ifndef GETALPHAS_H
#define GETALPHAS_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
extern void getAlphas(const double vl1_data[], const int vl1_size[1],
                      const double vl2_data[], const int vl2_size[1],
                      const double vla_data[], const int vla_size[1],
                      const double vlc_data[], const int vlc_size[1],
                      boolean_T use_padding, double alphaE[4][4], double stE[4],
                      double smE[4]);

#endif
// End of code generation (getAlphas.h)
