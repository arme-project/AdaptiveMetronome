//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// diff.cpp
//
// Code generation for function 'diff'
//

// Include files
#include "diff.h"
#include "rt_nonfinite.h"

// Function Definitions
//
//
namespace coder {
void diff(const double x_data[], const int x_size[2], double y_data[],
          int y_size[2])
{
  int dimSize;
  dimSize = x_size[0];
  if (x_size[0] == 0) {
    y_size[0] = 0;
    y_size[1] = 4;
  } else {
    int iyStart;
    iyStart = x_size[0] - 1;
    if (iyStart > 1) {
      iyStart = 1;
    }
    if (iyStart < 1) {
      y_size[0] = 0;
      y_size[1] = 4;
    } else {
      y_size[0] = x_size[0] - 1;
      y_size[1] = 4;
      if (x_size[0] - 1 != 0) {
        iyStart = 0;
        for (int r{0}; r < 4; r++) {
          double work_data;
          int ixLead_tmp;
          ixLead_tmp = r * dimSize;
          work_data = x_data[ixLead_tmp];
          for (int m{2}; m <= dimSize; m++) {
            double d;
            double tmp1;
            tmp1 = x_data[(ixLead_tmp + m) - 1];
            d = tmp1;
            tmp1 -= work_data;
            work_data = d;
            y_data[(iyStart + m) - 2] = tmp1;
          }
          iyStart = (iyStart + dimSize) - 1;
        }
      }
    }
  }
}

} // namespace coder

// End of code generation (diff.cpp)
