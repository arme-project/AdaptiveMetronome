//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xgetrf.cpp
//
// Code generation for function 'xgetrf'
//

// Include files
#include "xgetrf.h"
#include <cmath>

// Function Definitions
namespace coder {
namespace internal {
namespace lapack {
int xgetrf(double A[45][45], int ipiv[45])
{
  int i;
  int info;
  for (i = 0; i < 45; i++) {
    ipiv[i] = i + 1;
  }
  info = 0;
  for (int j{0}; j < 44; j++) {
    double smax;
    int a;
    int b_tmp;
    int jA;
    int jp1j;
    int mmj_tmp;
    mmj_tmp = 43 - j;
    b_tmp = j * 46;
    jp1j = b_tmp + 2;
    jA = 45 - j;
    a = 0;
    smax = std::abs((&A[0][0])[b_tmp]);
    for (int k{2}; k <= jA; k++) {
      double s;
      s = std::abs((&A[0][0])[(b_tmp + k) - 1]);
      if (s > smax) {
        a = k - 1;
        smax = s;
      }
    }
    if ((&A[0][0])[b_tmp + a] != 0.0) {
      if (a != 0) {
        jA = j + a;
        ipiv[j] = jA + 1;
        for (int k{0}; k < 45; k++) {
          a = j + k * 45;
          smax = (&A[0][0])[a];
          i = jA + k * 45;
          (&A[0][0])[a] = (&A[0][0])[i];
          (&A[0][0])[i] = smax;
        }
      }
      i = (b_tmp - j) + 45;
      for (jA = jp1j; jA <= i; jA++) {
        (&A[0][0])[jA - 1] /= (&A[0][0])[b_tmp];
      }
    } else {
      info = j + 1;
    }
    jA = b_tmp;
    for (jp1j = 0; jp1j <= mmj_tmp; jp1j++) {
      smax = (&A[0][0])[(b_tmp + jp1j * 45) + 45];
      if (smax != 0.0) {
        i = jA + 47;
        a = (jA - j) + 90;
        for (int k{i}; k <= a; k++) {
          (&A[0][0])[k - 1] += (&A[0][0])[((b_tmp + k) - jA) - 46] * -smax;
        }
      }
      jA += 45;
    }
  }
  if ((info == 0) && (!(A[44][44] != 0.0))) {
    info = 45;
  }
  return info;
}

} // namespace lapack
} // namespace internal
} // namespace coder

// End of code generation (xgetrf.cpp)
