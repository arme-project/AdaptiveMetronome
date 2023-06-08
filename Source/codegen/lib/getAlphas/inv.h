//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// inv.h
//
// Code generation for function 'inv'
//

#ifndef INV_H
#define INV_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
namespace coder {
void b_inv(const double x[18][18], double y[18][18]);

void inv(const double x_data[], const int x_size[2], double y_data[],
         int y_size[2]);

void inv(const double x[3][3], double y[3][3]);

} // namespace coder

#endif
// End of code generation (inv.h)
