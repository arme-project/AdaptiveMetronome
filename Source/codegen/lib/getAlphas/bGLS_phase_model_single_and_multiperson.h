//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// bGLS_phase_model_single_and_multiperson.h
//
// Code generation for function 'bGLS_phase_model_single_and_multiperson'
//

#ifndef BGLS_PHASE_MODEL_SINGLE_AND_MULTIPERSON_H
#define BGLS_PHASE_MODEL_SINGLE_AND_MULTIPERSON_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
void binary_expand_op(double in1_data[], int in1_size[2],
                      const double in2_data[], const int in2_size[2],
                      const double in3_data[], const int in3_size[2]);

int minus(double in1_data[], const double in2_data[], const int &in2_size,
          const double in3_data[], const int &in3_size);

#endif
// End of code generation (bGLS_phase_model_single_and_multiperson.h)
