//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// cov.cpp
//
// Code generation for function 'cov'
//

// Include files
#include "cov.h"
#include "rt_nonfinite.h"
#include <algorithm>
#include <cmath>

// Function Definitions
//
//
namespace coder {
void cov(const double x_data[], int x_size, const double varargin_1_data[],
         int varargin_1_size, double xy[2][2])
{
  double result_data[36];
  xy[0][0] = 0.0;
  xy[0][1] = 0.0;
  xy[1][0] = 0.0;
  xy[1][1] = 0.0;
  if ((x_size == 1) && (varargin_1_size == 1)) {
    if (std::isinf(x_data[0]) || std::isnan(x_data[0])) {
      xy[0][0] = rtNaN;
      xy[0][1] = rtNaN;
      xy[1][0] = rtNaN;
    }
    if (std::isinf(varargin_1_data[0]) || std::isnan(varargin_1_data[0])) {
      xy[0][1] = rtNaN;
      xy[1][0] = rtNaN;
      xy[1][1] = rtNaN;
    }
  } else {
    double tmp_data[4];
    int i;
    int result_data_tmp;
    if (x_size - 1 >= 0) {
      std::copy(&x_data[0], &x_data[x_size], &result_data[0]);
    }
    for (i = 0; i < varargin_1_size; i++) {
      result_data[i + x_size] = varargin_1_data[i];
    }
    if (x_size == 1) {
      double d;
      double muj;
      double temp;
      muj = (result_data[0] + result_data[1]) / 2.0;
      d = result_data[0] - muj;
      temp = d * d;
      d = result_data[1] - muj;
      temp += d * d;
      result_data_tmp = 1;
      tmp_data[0] = temp;
    } else {
      double C[2][2];
      int m;
      m = x_size - 1;
      C[0][0] = 0.0;
      C[0][1] = 0.0;
      C[1][0] = 0.0;
      C[1][1] = 0.0;
      if (x_size == 0) {
        C[0][0] = rtNaN;
        C[0][1] = rtNaN;
        C[1][0] = rtNaN;
        C[1][1] = rtNaN;
      } else if (x_size >= 2) {
        double muj;
        int ar;
        int br;
        for (br = 0; br < 2; br++) {
          muj = 0.0;
          for (ar = 0; ar <= m; ar++) {
            muj += result_data[ar + x_size * br];
          }
          muj /= static_cast<double>(x_size);
          for (ar = 0; ar <= m; ar++) {
            result_data_tmp = ar + x_size * br;
            result_data[result_data_tmp] -= muj;
          }
        }
        muj = 1.0 / (static_cast<double>(x_size) - 1.0);
        for (int cr{0}; cr <= 2; cr += 2) {
          i = cr + 1;
          result_data_tmp = cr + 2;
          for (int ic{i}; ic <= result_data_tmp; ic++) {
            (&C[0][0])[ic - 1] = 0.0;
          }
        }
        br = -1;
        for (int cr{0}; cr <= 2; cr += 2) {
          ar = -1;
          i = cr + 1;
          result_data_tmp = cr + 2;
          for (int ic{i}; ic <= result_data_tmp; ic++) {
            double temp;
            temp = 0.0;
            for (int w{0}; w <= m; w++) {
              temp += result_data[(w + ar) + 1] * result_data[(w + br) + 1];
            }
            (&C[0][0])[ic - 1] += muj * temp;
            ar += x_size;
          }
          br += x_size;
        }
      }
      result_data_tmp = 2;
      tmp_data[0] = C[0][0];
      tmp_data[1] = C[0][1];
      tmp_data[2] = C[1][0];
      tmp_data[3] = C[1][1];
    }
    xy[0][0] = tmp_data[0];
    xy[0][1] = tmp_data[1];
    xy[1][0] = tmp_data[result_data_tmp];
    xy[1][1] = tmp_data[result_data_tmp + 1];
  }
}

} // namespace coder

// End of code generation (cov.cpp)
