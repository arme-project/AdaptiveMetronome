//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// bGLS_phase_model_single_and_multiperson.cpp
//
// Code generation for function 'bGLS_phase_model_single_and_multiperson'
//

// Include files
#include "bGLS_phase_model_single_and_multiperson.h"
#include "inv.h"
#include "rt_nonfinite.h"

// Function Definitions
void binary_expand_op(double in1_data[], int in1_size[2],
                      const double in2_data[], const int in2_size[2],
                      const double in3_data[], const int in3_size[2])
{
  double b_in1_data[324];
  int b_in1_size[2];
  int aux_0_1;
  int aux_1_1;
  int aux_2_1;
  int b_loop_ub;
  int loop_ub;
  int stride_0_0;
  int stride_0_1;
  int stride_1_0;
  int stride_1_1;
  int stride_2_0;
  int stride_2_1;
  if (in3_size[0] == 1) {
    if (in2_size[0] == 1) {
      loop_ub = in1_size[0];
    } else {
      loop_ub = in2_size[0];
    }
  } else {
    loop_ub = in3_size[0];
  }
  b_in1_size[0] = loop_ub;
  if (in3_size[1] == 1) {
    if (in2_size[1] == 1) {
      b_loop_ub = in1_size[1];
    } else {
      b_loop_ub = in2_size[1];
    }
  } else {
    b_loop_ub = in3_size[1];
  }
  b_in1_size[1] = b_loop_ub;
  stride_0_0 = (in1_size[0] != 1);
  stride_0_1 = (in1_size[1] != 1);
  stride_1_0 = (in2_size[0] != 1);
  stride_1_1 = (in2_size[1] != 1);
  stride_2_0 = (in3_size[0] != 1);
  stride_2_1 = (in3_size[1] != 1);
  aux_0_1 = 0;
  aux_1_1 = 0;
  aux_2_1 = 0;
  for (int i{0}; i < b_loop_ub; i++) {
    for (int i1{0}; i1 < loop_ub; i1++) {
      b_in1_data[i1 + loop_ub * i] =
          (in1_data[i1 * stride_0_0 + in1_size[0] * aux_0_1] +
           in2_data[i1 * stride_1_0 + in2_size[0] * aux_1_1]) +
          in3_data[i1 * stride_2_0 + in3_size[0] * aux_2_1];
    }
    aux_2_1 += stride_2_1;
    aux_1_1 += stride_1_1;
    aux_0_1 += stride_0_1;
  }
  coder::inv(b_in1_data, b_in1_size, in1_data, in1_size);
}

int minus(double in1_data[], const double in2_data[], const int &in2_size,
          const double in3_data[], const int &in3_size)
{
  int in1_size;
  int stride_0_0;
  int stride_1_0;
  if (in3_size == 1) {
    in1_size = in2_size;
  } else {
    in1_size = in3_size;
  }
  stride_0_0 = (in2_size != 1);
  stride_1_0 = (in3_size != 1);
  for (int i{0}; i < in1_size; i++) {
    in1_data[i] = in2_data[i * stride_0_0] - in3_data[i * stride_1_0];
  }
  return in1_size;
}

// End of code generation (bGLS_phase_model_single_and_multiperson.cpp)
