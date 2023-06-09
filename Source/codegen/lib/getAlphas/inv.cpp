//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// inv.cpp
//
// Code generation for function 'inv'
//

// Include files
#include "inv.h"
#include "rt_nonfinite.h"
#include <cmath>

// Function Definitions
//
//
namespace coder {
void b_inv(const double x[18][18], double y[18][18])
{
  double b_x[18][18];
  double smax;
  int i;
  int i1;
  int jA;
  int kAcol;
  signed char ipiv[18];
  signed char p[18];
  for (i = 0; i < 18; i++) {
    for (i1 = 0; i1 < 18; i1++) {
      y[i][i1] = 0.0;
      b_x[i][i1] = x[i][i1];
    }
    ipiv[i] = static_cast<signed char>(i + 1);
  }
  for (int j{0}; j < 17; j++) {
    int b_tmp;
    int jp1j;
    int mmj_tmp;
    mmj_tmp = 16 - j;
    b_tmp = j * 19;
    jp1j = b_tmp + 2;
    jA = 18 - j;
    kAcol = 0;
    smax = std::abs((&b_x[0][0])[b_tmp]);
    for (int k{2}; k <= jA; k++) {
      double s;
      s = std::abs((&b_x[0][0])[(b_tmp + k) - 1]);
      if (s > smax) {
        kAcol = k - 1;
        smax = s;
      }
    }
    if ((&b_x[0][0])[b_tmp + kAcol] != 0.0) {
      if (kAcol != 0) {
        jA = j + kAcol;
        ipiv[j] = static_cast<signed char>(jA + 1);
        for (int k{0}; k < 18; k++) {
          kAcol = j + k * 18;
          smax = (&b_x[0][0])[kAcol];
          i = jA + k * 18;
          (&b_x[0][0])[kAcol] = (&b_x[0][0])[i];
          (&b_x[0][0])[i] = smax;
        }
      }
      i = (b_tmp - j) + 18;
      for (int b_i{jp1j}; b_i <= i; b_i++) {
        (&b_x[0][0])[b_i - 1] /= (&b_x[0][0])[b_tmp];
      }
    }
    jA = b_tmp;
    for (kAcol = 0; kAcol <= mmj_tmp; kAcol++) {
      smax = (&b_x[0][0])[(b_tmp + kAcol * 18) + 18];
      if (smax != 0.0) {
        i = jA + 20;
        i1 = (jA - j) + 36;
        for (jp1j = i; jp1j <= i1; jp1j++) {
          (&b_x[0][0])[jp1j - 1] +=
              (&b_x[0][0])[((b_tmp + jp1j) - jA) - 19] * -smax;
        }
      }
      jA += 18;
    }
  }
  for (i = 0; i < 18; i++) {
    p[i] = static_cast<signed char>(i + 1);
  }
  for (int k{0}; k < 17; k++) {
    signed char i2;
    i2 = ipiv[k];
    if (i2 > k + 1) {
      jA = p[i2 - 1];
      p[i2 - 1] = p[k];
      p[k] = static_cast<signed char>(jA);
    }
  }
  for (int k{0}; k < 18; k++) {
    jA = p[k] - 1;
    y[jA][k] = 1.0;
    for (int j{k + 1}; j < 19; j++) {
      if (y[jA][j - 1] != 0.0) {
        i = j + 1;
        for (int b_i{i}; b_i < 19; b_i++) {
          y[jA][b_i - 1] -= y[jA][j - 1] * b_x[j - 1][b_i - 1];
        }
      }
    }
  }
  for (int j{0}; j < 18; j++) {
    jA = 18 * j;
    for (int k{17}; k >= 0; k--) {
      kAcol = 18 * k;
      i = k + jA;
      smax = (&y[0][0])[i];
      if (smax != 0.0) {
        (&y[0][0])[i] = smax / (&b_x[0][0])[k + kAcol];
        for (int b_i{0}; b_i < k; b_i++) {
          i1 = b_i + jA;
          (&y[0][0])[i1] -= (&y[0][0])[i] * (&b_x[0][0])[b_i + kAcol];
        }
      }
    }
  }
}

//
//
void inv(const double x_data[], const int x_size[2], double y_data[],
         int y_size[2])
{
  if ((x_size[0] == 0) || (x_size[1] == 0)) {
    int b_n;
    y_size[0] = x_size[0];
    y_size[1] = x_size[1];
    b_n = x_size[1];
    for (int i{0}; i < b_n; i++) {
      int yk;
      yk = x_size[0];
      for (int i1{0}; i1 < yk; i1++) {
        y_data[i1 + y_size[0] * i] = x_data[i1 + x_size[0] * i];
      }
    }
  } else {
    double b_x_data[324];
    int b_n;
    int i;
    int i1;
    int ipiv_size_idx_1;
    int jA;
    int n;
    int u1;
    int x_size_idx_0;
    int yk;
    signed char ipiv_data[18];
    signed char p_data[18];
    signed char i2;
    n = x_size[0];
    y_size[0] = x_size[0];
    y_size[1] = x_size[1];
    b_n = x_size[1];
    x_size_idx_0 = x_size[0];
    yk = x_size[0];
    for (i = 0; i < b_n; i++) {
      for (i1 = 0; i1 < yk; i1++) {
        y_data[i1 + y_size[0] * i] = 0.0;
        b_x_data[i1 + x_size_idx_0 * i] = x_data[i1 + x_size[0] * i];
      }
    }
    b_n = x_size[0];
    ipiv_size_idx_1 = x_size[0];
    ipiv_data[0] = 1;
    yk = 1;
    for (int k{2}; k <= b_n; k++) {
      yk++;
      ipiv_data[k - 1] = static_cast<signed char>(yk);
    }
    b_n = x_size[0] - 1;
    u1 = x_size[0];
    if (b_n <= u1) {
      u1 = b_n;
    }
    for (int j{0}; j < u1; j++) {
      double smax;
      int b_tmp;
      int jp1j;
      int mmj_tmp;
      mmj_tmp = n - j;
      b_tmp = j * (n + 1);
      jp1j = b_tmp + 2;
      if (mmj_tmp < 1) {
        b_n = -1;
      } else {
        b_n = 0;
        if (mmj_tmp > 1) {
          smax = std::abs(b_x_data[b_tmp]);
          for (int k{2}; k <= mmj_tmp; k++) {
            double s;
            s = std::abs(b_x_data[(b_tmp + k) - 1]);
            if (s > smax) {
              b_n = k - 1;
              smax = s;
            }
          }
        }
      }
      if (b_x_data[b_tmp + b_n] != 0.0) {
        if (b_n != 0) {
          yk = j + b_n;
          ipiv_data[j] = static_cast<signed char>(yk + 1);
          for (int k{0}; k < n; k++) {
            b_n = k * n;
            jA = j + b_n;
            smax = b_x_data[jA];
            i = yk + b_n;
            b_x_data[jA] = b_x_data[i];
            b_x_data[i] = smax;
          }
        }
        i = b_tmp + mmj_tmp;
        for (jA = jp1j; jA <= i; jA++) {
          b_x_data[jA - 1] /= b_x_data[b_tmp];
        }
      }
      yk = b_tmp + n;
      jA = yk;
      for (jp1j = 0; jp1j <= mmj_tmp - 2; jp1j++) {
        b_n = yk + jp1j * n;
        smax = b_x_data[b_n];
        if (b_x_data[b_n] != 0.0) {
          i = jA + 2;
          i1 = mmj_tmp + jA;
          for (b_n = i; b_n <= i1; b_n++) {
            b_x_data[b_n - 1] += b_x_data[((b_tmp + b_n) - jA) - 1] * -smax;
          }
        }
        jA += n;
      }
    }
    b_n = x_size[0];
    p_data[0] = 1;
    yk = 1;
    for (int k{2}; k <= b_n; k++) {
      yk++;
      p_data[k - 1] = static_cast<signed char>(yk);
    }
    for (int k{0}; k < ipiv_size_idx_1; k++) {
      i2 = ipiv_data[k];
      if (i2 > k + 1) {
        b_n = p_data[i2 - 1];
        p_data[i2 - 1] = p_data[k];
        p_data[k] = static_cast<signed char>(b_n);
      }
    }
    for (int k{0}; k < n; k++) {
      i2 = p_data[k];
      y_data[k + y_size[0] * (i2 - 1)] = 1.0;
      for (int j{k + 1}; j <= n; j++) {
        if (y_data[(j + y_size[0] * (i2 - 1)) - 1] != 0.0) {
          i = j + 1;
          for (jA = i; jA <= n; jA++) {
            b_n = (jA + y_size[0] * (i2 - 1)) - 1;
            y_data[b_n] -= y_data[(j + y_size[0] * (i2 - 1)) - 1] *
                           b_x_data[(jA + x_size_idx_0 * (j - 1)) - 1];
          }
        }
      }
    }
    for (int j{0}; j < n; j++) {
      b_n = n * j - 1;
      for (int k{n}; k >= 1; k--) {
        yk = n * (k - 1) - 1;
        i = k + b_n;
        if (y_data[i] != 0.0) {
          y_data[i] /= b_x_data[k + yk];
          for (jA = 0; jA <= k - 2; jA++) {
            i1 = (jA + b_n) + 1;
            y_data[i1] -= y_data[i] * b_x_data[(jA + yk) + 1];
          }
        }
      }
    }
  }
}

//
//
void inv(const double x[3][3], double y[3][3])
{
  double b_x[3][3];
  double absx11;
  double absx21;
  double absx31;
  int p1;
  int p2;
  int p3;
  for (p1 = 0; p1 < 3; p1++) {
    b_x[p1][0] = x[p1][0];
    b_x[p1][1] = x[p1][1];
    b_x[p1][2] = x[p1][2];
  }
  p1 = 0;
  p2 = 3;
  p3 = 6;
  absx11 = std::abs(x[0][0]);
  absx21 = std::abs(x[0][1]);
  absx31 = std::abs(x[0][2]);
  if ((absx21 > absx11) && (absx21 > absx31)) {
    p1 = 3;
    p2 = 0;
    b_x[0][0] = x[0][1];
    b_x[0][1] = x[0][0];
    b_x[1][0] = x[1][1];
    b_x[1][1] = x[1][0];
    b_x[2][0] = x[2][1];
    b_x[2][1] = x[2][0];
  } else if (absx31 > absx11) {
    p1 = 6;
    p3 = 0;
    b_x[0][0] = x[0][2];
    b_x[0][2] = x[0][0];
    b_x[1][0] = x[1][2];
    b_x[1][2] = x[1][0];
    b_x[2][0] = x[2][2];
    b_x[2][2] = x[2][0];
  }
  b_x[0][1] /= b_x[0][0];
  b_x[0][2] /= b_x[0][0];
  b_x[1][1] -= b_x[1][0] * b_x[0][1];
  b_x[1][2] -= b_x[1][0] * b_x[0][2];
  b_x[2][1] -= b_x[2][0] * b_x[0][1];
  b_x[2][2] -= b_x[2][0] * b_x[0][2];
  if (std::abs(b_x[1][2]) > std::abs(b_x[1][1])) {
    int itmp;
    itmp = p2;
    p2 = p3;
    p3 = itmp;
    absx11 = b_x[0][1];
    b_x[0][1] = b_x[0][2];
    b_x[0][2] = absx11;
    absx11 = b_x[1][1];
    b_x[1][1] = b_x[1][2];
    b_x[1][2] = absx11;
    absx11 = b_x[2][1];
    b_x[2][1] = b_x[2][2];
    b_x[2][2] = absx11;
  }
  b_x[1][2] /= b_x[1][1];
  b_x[2][2] -= b_x[2][1] * b_x[1][2];
  absx11 = (b_x[0][1] * b_x[1][2] - b_x[0][2]) / b_x[2][2];
  absx21 = -(b_x[0][1] + b_x[2][1] * absx11) / b_x[1][1];
  (&y[0][0])[p1] =
      ((1.0 - b_x[1][0] * absx21) - b_x[2][0] * absx11) / b_x[0][0];
  (&y[0][0])[p1 + 1] = absx21;
  (&y[0][0])[p1 + 2] = absx11;
  absx11 = -b_x[1][2] / b_x[2][2];
  absx21 = (1.0 - b_x[2][1] * absx11) / b_x[1][1];
  (&y[0][0])[p2] = -(b_x[1][0] * absx21 + b_x[2][0] * absx11) / b_x[0][0];
  (&y[0][0])[p2 + 1] = absx21;
  (&y[0][0])[p2 + 2] = absx11;
  absx11 = 1.0 / b_x[2][2];
  absx21 = -b_x[2][1] * absx11 / b_x[1][1];
  (&y[0][0])[p3] = -(b_x[1][0] * absx21 + b_x[2][0] * absx11) / b_x[0][0];
  (&y[0][0])[p3 + 1] = absx21;
  (&y[0][0])[p3 + 2] = absx11;
}

} // namespace coder

// End of code generation (inv.cpp)
