//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// bgls_from_onsets_and_window.h
//
// Code generation for function 'bgls_from_onsets_and_window'
//

#ifndef BGLS_FROM_ONSETS_AND_WINDOW_H
#define BGLS_FROM_ONSETS_AND_WINDOW_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
void bgls_from_onsets_and_window(const double Rm_data[], const int Rm_size[2],
                                 const double Am_data[], const int Am_size[3],
                                 double b_I, double alphaE[4][4], double stE[4],
                                 double smE[4]);

#endif
// End of code generation (bgls_from_onsets_and_window.h)
