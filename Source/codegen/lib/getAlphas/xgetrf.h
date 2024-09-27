//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xgetrf.h
//
// Code generation for function 'xgetrf'
//

#ifndef XGETRF_H
#define XGETRF_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
namespace coder {
namespace internal {
namespace lapack {
int xgetrf(double A[45][45], int ipiv[45]);

}
} // namespace internal
} // namespace coder

#endif
// End of code generation (xgetrf.h)
