//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// eml_setop.cpp
//
// Code generation for function 'eml_setop'
//

// Include files
#include "eml_setop.h"
#include "rt_nonfinite.h"

// Function Definitions
//
//
namespace coder {
int do_vectors(double b, double c_data[], int c_size[2], int ia_data[],
               int &ib_size)
{
  int b_ialast;
  int ia_size;
  int iafirst;
  int ialast;
  int iblast;
  int nc;
  int nia;
  c_size[0] = 1;
  ib_size = 0;
  nc = 0;
  nia = -1;
  iafirst = 0;
  ialast = 1;
  iblast = 1;
  while ((ialast <= 4) && (iblast <= 1)) {
    int ak;
    b_ialast = ialast;
    ak = ialast;
    while ((b_ialast < 4) && (b_ialast + 1 == ialast)) {
      b_ialast++;
    }
    ialast = b_ialast;
    if (ak == b) {
      ialast = b_ialast + 1;
      iafirst = b_ialast;
      iblast = 2;
    } else if (ak < b) {
      nc++;
      nia++;
      c_data[nc - 1] = ak;
      ia_data[nia] = iafirst + 1;
      ialast = b_ialast + 1;
      iafirst = b_ialast;
    } else {
      iblast = 2;
    }
  }
  while (ialast <= 4) {
    b_ialast = ialast;
    while ((b_ialast < 4) && (b_ialast + 1 == ialast)) {
      b_ialast++;
    }
    nc++;
    nia++;
    c_data[nc - 1] = (static_cast<double>(ialast) - 1.0) + 1.0;
    ia_data[nia] = iafirst + 1;
    ialast = b_ialast + 1;
    iafirst = b_ialast;
  }
  if (nia + 1 < 1) {
    nia = -1;
  }
  ia_size = nia + 1;
  if (nc < 1) {
    c_size[1] = 0;
  } else {
    c_size[1] = nc;
  }
  return ia_size;
}

} // namespace coder

// End of code generation (eml_setop.cpp)
