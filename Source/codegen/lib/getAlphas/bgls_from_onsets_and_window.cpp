//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// bgls_from_onsets_and_window.cpp
//
// Code generation for function 'bgls_from_onsets_and_window'
//

// Include files
#include "bgls_from_onsets_and_window.h"
#include "bGLS_phase_model_single_and_multiperson.h"
#include "cov.h"
#include "eml_setop.h"
#include "inv.h"
#include "rt_nonfinite.h"
#include <algorithm>
#include <cmath>

// Function Definitions
//
// function [alphaE,stE,smE] = bgls_from_onsets_and_window(Rm, Am, ORm, I,
// W_SIZE)
void bgls_from_onsets_and_window(const double Rm_data[], const int Rm_size[2],
                                 const double Am_data[], const int Am_size[3],
                                 double b_I, double alphaE[4][4], double stE[4],
                                 double smE[4])
{
  double d_data[19];
  double y_data[18];
  double others_data[4];
  double z[3];
  int ia_data[4];
  int B_size[2];
  int b_B_size[2];
  int tmp_size[2];
  int b3_size;
  int i;
  int ia_size;
  int k;
  int loop_ub;
  int loop_ub_tmp;
  int m;
  int nv;
  //  Initialise trial estimates
  // 'bgls_from_onsets_and_window:4' alphaE = zeros(4,4);
  for (i = 0; i < 4; i++) {
    alphaE[i][0] = 0.0;
    alphaE[i][1] = 0.0;
    alphaE[i][2] = 0.0;
    alphaE[i][3] = 0.0;
  }
  // 'bgls_from_onsets_and_window:5' smE = zeros(4,1);
  // 'bgls_from_onsets_and_window:6' stE = zeros(4,1);
  //  Grab the appropriate window from Rm/Am/ORm
  //  Am is just an S_SIZE x 1 matrix, so grab the appropriate
  //  rows
  //  display(max(I-W_SIZE, 1));
  //  wAm = Am(max(I-W_SIZE, 1):I,:,:);
  // 'bgls_from_onsets_and_window:13' wAm = Am(max(I-W_SIZE, 1):I,:,:);
  if (b_I < 1.0) {
    loop_ub = 0;
  } else {
    loop_ub = static_cast<int>(b_I);
  }
  //  Rm is S_SIZE x 4, so grab the appropriate rows, and all 4
  //  columns
  // 'bgls_from_onsets_and_window:17' wRm = Rm(max(I-W_SIZE, 1):I, 1:4);
  //  Do the same for ORm
  // 'bgls_from_onsets_and_window:20' wORm = ORm(max(I-W_SIZE, 1):I, 1:4);
  // 'bgls_from_onsets_and_window:22' for SUBJ=1:4
  if (loop_ub < 2) {
    i = 0;
    k = -1;
  } else {
    i = 1;
    k = loop_ub - 1;
  }
  loop_ub_tmp = k - i;
  b3_size = loop_ub_tmp + 1;
  if (loop_ub - 1 < 1) {
    m = -1;
  } else {
    m = loop_ub - 2;
  }
  for (int SUBJ{0}; SUBJ < 4; SUBJ++) {
    double As_data[57];
    double b3_data[19];
    double MEAN_A[3];
    double K11;
    double K12;
    double zold_idx_0;
    double zold_idx_1;
    double zold_idx_2;
    int K;
    int iter;
    int y_tmp_data_tmp;
    boolean_T exitg1;
    //   prepare dataset
    // 'bgls_from_onsets_and_window:25' R = wRm(:,SUBJ);
    //  R is the IOIs for SUBJ (NB: first row is 170s b/c of 'zero point' where
    //  each onset starts after the tempo in ms)
    // 'bgls_from_onsets_and_window:26' As = zeros(size(R,1),3);
    for (k = 0; k < 3; k++) {
      for (y_tmp_data_tmp = 0; y_tmp_data_tmp < loop_ub; y_tmp_data_tmp++) {
        As_data[y_tmp_data_tmp + loop_ub * k] = 0.0;
      }
    }
    int others_size[2];
    //  size(R, 1) is the number of rows in R,
    // 'bgls_from_onsets_and_window:27' others = setdiff(1:4,SUBJ);
    coder::do_vectors(static_cast<double>(SUBJ) + 1.0, others_data, others_size,
                      ia_data, nv);
    //  numbers that aren't the current SUBJ index
    // 'bgls_from_onsets_and_window:28' assert(length(others) == 3);
    // 'bgls_from_onsets_and_window:29' for K=1:length(wAm)
    ia_size = loop_ub;
    if ((loop_ub > 0) && (loop_ub < 4)) {
      ia_size = 4;
    }
    if ((ia_size > 0) && (ia_size < 4)) {
      ia_size = 4;
    }
    for (K = 0; K < ia_size; K++) {
      //  Iterates through Am, which is the matrix of asynchronies at each point
      // 'bgls_from_onsets_and_window:30' As(K,1) = wAm(K, SUBJ , others(1));
      nv = K + Am_size[0] * SUBJ;
      As_data[K] =
          Am_data[nv + Am_size[0] * 4 * (static_cast<int>(others_data[0]) - 1)];
      //  As is the asynchronies between SUBJ and the other three players (Ks)
      // 'bgls_from_onsets_and_window:31' As(K,2) = wAm(K, SUBJ , others(2));
      As_data[K + loop_ub] =
          Am_data[nv + Am_size[0] * 4 * (static_cast<int>(others_data[1]) - 1)];
      //  Thus you get a (#players - 1) x N matrix
      // 'bgls_from_onsets_and_window:32' As(K,3) = wAm(K, SUBJ , others(3));
      As_data[K + loop_ub * 2] =
          Am_data[nv + Am_size[0] * 4 * (static_cast<int>(others_data[2]) - 1)];
    }
    //  do bGLS for each player
    // 'bgls_from_onsets_and_window:36' MEAN_A = mean(As);
    if (loop_ub == 0) {
      MEAN_A[0] = 0.0;
      MEAN_A[1] = 0.0;
      MEAN_A[2] = 0.0;
    } else {
      for (k = 0; k < 3; k++) {
        ia_size = loop_ub * k;
        MEAN_A[k] = As_data[ia_size];
        for (nv = 2; nv <= loop_ub; nv++) {
          if (loop_ub >= 2) {
            MEAN_A[k] += As_data[(nv + ia_size) - 1];
          }
        }
      }
    }
    MEAN_A[0] /= static_cast<double>(loop_ub);
    MEAN_A[1] /= static_cast<double>(loop_ub);
    MEAN_A[2] /= static_cast<double>(loop_ub);
    //  Calculate the average asynchrony for SUBJ
    // 'bgls_from_onsets_and_window:37' MEAN_R = mean(R);
    if (loop_ub == 0) {
      K12 = 0.0;
    } else {
      ia_size = Rm_size[0] * SUBJ;
      K12 = Rm_data[ia_size];
      for (k = 2; k <= loop_ub; k++) {
        if (loop_ub >= 2) {
          K12 += Rm_data[(k + ia_size) - 1];
        }
      }
    }
    K12 /= static_cast<double>(loop_ub);
    //  Calculate the average IOI for SUBJ
    // 'bgls_from_onsets_and_window:38' [alphas,st,sm] =
    // bGLS_phase_model_single_and_multiperson(R,As,MEAN_A,MEAN_R);
    // %%%%%%%%%%%%%%%%%%
    //  This method computes the bGLS for single and multiple subject.
    //  it is the function to use when there are no signficant tempo cahnges.
    //
    //    For more information see: Nori Jacoby, Naftali Tishby, Bruno H. Repp,
    //    Merav Ahissar and Peter E. Keller (2015)
    //
    //  Let OR(t) be the response onset at time t
    //  Let OS(t) be the stimulus onset at time t
    //  Let R(t) be the interesponse interval R(t)=OR(t)-OR(t-1)
    //  note that R is is slightly diffrent than the notation of
    //  Vorberg and Shultze 2002 where:
    //  I(t)=OR(t+1)-OR(t)=R(t+1)
    //  for multiple person the input of As is a N by P asynchronies
    //  R is again N by 1 vector (we look at one subject).
    //  the empirical means should be computed outside.
    //  =====  CODE BY: Nori Jacoby (nori.viola@gmail.com)
    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    //  ==========================================================================
    //  ===== For information please contact: Nori Jacoby
    //  ===== Nori.viola@gmail.com
    //  =====
    //  ===== If you are using the code,
    //  ===== Please cite this version:
    //  =====
    //  ===== Jacoby, Nori, Peter Keller, Bruno H. Repp, Merav Ahissar and
    //  Naftali Tishby.
    //  ===== "Parameter Estimation of Linear Sensorimotor Synchronization
    //  Models: Phase Correction, Period Correction and Ensemble
    //  Synchronization."
    //  ===== Special issue of Timing & Time Perception (RPPW).
    //  ==========================================================================
    // 'bGLS_phase_model_single_and_multiperson:31' ITER=20;
    //  set parameters
    // 'bGLS_phase_model_single_and_multiperson:32' TRESH=1e-3;
    // this is the maximal difference between old solution and new solution.
    //  in the iteration if we get a change smaller than TRESH we simply stop
    //  (we obtaine a local maximum).
    // 'bGLS_phase_model_single_and_multiperson:36' N=size(R,1)-1;
    // number of datapoints
    // 'bGLS_phase_model_single_and_multiperson:37' P=size(As,2);
    // number of partners
    //  !IMPORTANT!
    //  TODO: Tom come back here and put these back after the demo!
    // 'bGLS_phase_model_single_and_multiperson:41'
    // assert(size(R,1)==size(As,1));
    // 'bGLS_phase_model_single_and_multiperson:42' assert(size(MEAN_A,2)==P);
    // 'bGLS_phase_model_single_and_multiperson:43' assert(size(MEAN_A,1)==1);
    //  reduce mean
    // 'bGLS_phase_model_single_and_multiperson:46' for p=1:P
    for (ia_size = 0; ia_size < 3; ia_size++) {
      // 'bGLS_phase_model_single_and_multiperson:47' As(:,p)=As(:,p)-MEAN_A(p);
      for (k = 0; k < loop_ub; k++) {
        b3_data[k] = As_data[k + loop_ub * ia_size] - MEAN_A[ia_size];
      }
      for (k = 0; k < loop_ub; k++) {
        As_data[k + loop_ub * ia_size] = b3_data[k];
      }
    }
    //  compute matrices
    // 'bGLS_phase_model_single_and_multiperson:51' b3=R(2:end)-MEAN_R;
    for (k = 0; k <= loop_ub_tmp; k++) {
      b3_data[k] = Rm_data[(i + k) + Rm_size[0] * SUBJ] - K12;
    }
    // 'bGLS_phase_model_single_and_multiperson:52' A3=[As(1:(end-1),:)];
    //  init acvf
    // 'bGLS_phase_model_single_and_multiperson:55' K11=1;
    K11 = 1.0;
    // 'bGLS_phase_model_single_and_multiperson:56' K12=0;
    K12 = 0.0;
    // 'bGLS_phase_model_single_and_multiperson:58' zold=zeros(P,1)-9999;
    zold_idx_0 = -9999.0;
    zold_idx_1 = -9999.0;
    zold_idx_2 = -9999.0;
    // init to invalid value
    //  do the BGLS iterations
    // 'bGLS_phase_model_single_and_multiperson:61' for iter=1:ITER
    iter = 0;
    exitg1 = false;
    while ((!exitg1) && (iter < 20)) {
      double B_data[324];
      double b_B_data[324];
      double tmp_data[324];
      double y_tmp_data[54];
      double b_d_data[19];
      double dv[3][3];
      double y[3][3];
      double b_K[2][2];
      double d;
      // 'bGLS_phase_model_single_and_multiperson:62' CC=diag(K11*ones(1,N),0)+
      // diag(K12*ones(1,N-1),1) + diag(K12*ones(1,N-1),-1);
      B_size[0] = static_cast<signed char>(loop_ub - 1);
      B_size[1] = static_cast<signed char>(loop_ub - 1);
      ia_size = static_cast<signed char>(loop_ub - 1);
      for (k = 0; k < ia_size; k++) {
        for (y_tmp_data_tmp = 0; y_tmp_data_tmp < ia_size; y_tmp_data_tmp++) {
          B_data[y_tmp_data_tmp + static_cast<signed char>(loop_ub - 1) * k] =
              0.0;
        }
      }
      for (int j{0}; j <= loop_ub - 2; j++) {
        B_data[j + static_cast<signed char>(loop_ub - 1) * j] = K11;
      }
      ia_size = loop_ub - 2;
      for (k = 0; k < ia_size; k++) {
        y_data[k] = K12;
      }
      nv = loop_ub - 2;
      b_B_size[0] = loop_ub - 1;
      b_B_size[1] = loop_ub - 1;
      ia_size = loop_ub - 1;
      for (k = 0; k < ia_size; k++) {
        for (y_tmp_data_tmp = 0; y_tmp_data_tmp < ia_size; y_tmp_data_tmp++) {
          b_B_data[y_tmp_data_tmp + (loop_ub - 1) * k] = 0.0;
        }
      }
      K = loop_ub - 2;
      for (int j{0}; j < nv; j++) {
        b_B_data[j + (loop_ub - 1) * (j + 1)] = y_data[j];
        y_data[j] = K12;
      }
      tmp_size[0] = loop_ub - 1;
      tmp_size[1] = loop_ub - 1;
      for (k = 0; k < ia_size; k++) {
        for (y_tmp_data_tmp = 0; y_tmp_data_tmp < ia_size; y_tmp_data_tmp++) {
          tmp_data[y_tmp_data_tmp + (loop_ub - 1) * k] = 0.0;
        }
      }
      for (int j{0}; j < K; j++) {
        tmp_data[(j + (loop_ub - 1) * j) + 1] = y_data[j];
      }
      // 'bGLS_phase_model_single_and_multiperson:63' iC=inv(CC);
      // 'bGLS_phase_model_single_and_multiperson:64'
      // z=inv((A3')*iC*A3)*((A3')*iC*b3);
      if (static_cast<signed char>(loop_ub - 1) == 1) {
        k = loop_ub - 1;
      } else {
        k = static_cast<signed char>(loop_ub - 1);
      }
      if ((static_cast<signed char>(loop_ub - 1) == loop_ub - 1) &&
          (k == loop_ub - 1) && (k == loop_ub - 1)) {
        b_B_size[0] = static_cast<signed char>(loop_ub - 1);
        b_B_size[1] = static_cast<signed char>(loop_ub - 1);
        K = static_cast<signed char>(loop_ub - 1);
        ia_size = static_cast<signed char>(loop_ub - 1);
        for (k = 0; k < K; k++) {
          for (y_tmp_data_tmp = 0; y_tmp_data_tmp < ia_size; y_tmp_data_tmp++) {
            nv = y_tmp_data_tmp + static_cast<signed char>(loop_ub - 1) * k;
            b_B_data[nv] = (B_data[y_tmp_data_tmp +
                                   static_cast<signed char>(loop_ub - 1) * k] +
                            b_B_data[nv]) +
                           tmp_data[y_tmp_data_tmp + (loop_ub - 1) * k];
          }
        }
        coder::inv(b_B_data, b_B_size, B_data, B_size);
      } else {
        binary_expand_op(B_data, B_size, b_B_data, b_B_size, tmp_data,
                         tmp_size);
      }
      ia_size = B_size[1];
      K = B_size[1];
      for (int j{0}; j < ia_size; j++) {
        y_tmp_data[3 * j] = 0.0;
        nv = 3 * j + 1;
        y_tmp_data[nv] = 0.0;
        y_tmp_data_tmp = 3 * j + 2;
        y_tmp_data[y_tmp_data_tmp] = 0.0;
        for (k = 0; k <= m; k++) {
          K12 = B_data[k + B_size[0] * j];
          y_tmp_data[3 * j] += As_data[k] * K12;
          y_tmp_data[nv] += As_data[k + loop_ub] * K12;
          y_tmp_data[y_tmp_data_tmp] += As_data[k + loop_ub * 2] * K12;
        }
      }
      for (int j{0}; j < 3; j++) {
        y[j][0] = 0.0;
        y[j][1] = 0.0;
        y[j][2] = 0.0;
        for (k = 0; k < K; k++) {
          K12 = As_data[k + loop_ub * j];
          y[j][0] += y_tmp_data[3 * k] * K12;
          y[j][1] += y_tmp_data[3 * k + 1] * K12;
          y[j][2] += y_tmp_data[3 * k + 2] * K12;
        }
        MEAN_A[j] = 0.0;
      }
      for (k = 0; k < K; k++) {
        K12 = b3_data[k];
        MEAN_A[0] += y_tmp_data[3 * k] * K12;
        MEAN_A[1] += y_tmp_data[3 * k + 1] * K12;
        MEAN_A[2] += y_tmp_data[3 * k + 2] * K12;
      }
      coder::inv(y, dv);
      K12 = MEAN_A[0];
      K11 = MEAN_A[1];
      d = MEAN_A[2];
      for (k = 0; k < 3; k++) {
        z[k] = (dv[0][k] * K12 + dv[1][k] * K11) + dv[2][k] * d;
      }
      //  compute GLS
      // 'bGLS_phase_model_single_and_multiperson:65' d=A3*z-b3;
      ia_size = m + 1;
      for (nv = 0; nv <= m; nv++) {
        y_data[nv] = (As_data[nv] * z[0] + As_data[nv + loop_ub] * z[1]) +
                     As_data[nv + loop_ub * 2] * z[2];
      }
      if (m + 1 == loop_ub_tmp + 1) {
        nv = m + 1;
        for (k = 0; k < ia_size; k++) {
          d_data[k] = y_data[k] - b3_data[k];
        }
      } else {
        nv = minus(d_data, y_data, ia_size, b3_data, b3_size);
      }
      //  compute residual noise
      // 'bGLS_phase_model_single_and_multiperson:67'
      // K=cov(d(1:(end-1)),d(2:end));
      if (nv - 1 < 1) {
        K = 0;
      } else {
        K = nv - 1;
      }
      if (nv < 2) {
        k = 0;
        nv = 0;
      } else {
        k = 1;
      }
      if (K - 1 >= 0) {
        std::copy(&d_data[0], &d_data[K], &y_data[0]);
      }
      ia_size = nv - k;
      for (y_tmp_data_tmp = 0; y_tmp_data_tmp < ia_size; y_tmp_data_tmp++) {
        b_d_data[y_tmp_data_tmp] = d_data[k + y_tmp_data_tmp];
      }
      coder::cov(y_data, K, b_d_data, ia_size, b_K);
      // estimate residual acvf
      // 'bGLS_phase_model_single_and_multiperson:68' K11=(K(1,1)+K(2,2))/2;
      K11 = (b_K[0][0] + b_K[1][1]) / 2.0;
      // 'bGLS_phase_model_single_and_multiperson:69' K12=K(1,2);
      K12 = b_K[1][0];
      //  apply bound
      // 'bGLS_phase_model_single_and_multiperson:72' if K12>0
      if (b_K[1][0] > 0.0) {
        // 'bGLS_phase_model_single_and_multiperson:73' K12=0;
        K12 = 0.0;
      }
      // 'bGLS_phase_model_single_and_multiperson:75' if K11<(-3*K12)
      if (K11 < -3.0 * K12) {
        // 'bGLS_phase_model_single_and_multiperson:76' K11=(-3*K12);
        K11 = -3.0 * K12;
      }
      //  if allready obtain local maxima there is no point to continue...
      // 'bGLS_phase_model_single_and_multiperson:80' if
      // (sum(abs(z-zold))<TRESH)
      if ((std::abs(z[0] - zold_idx_0) + std::abs(z[1] - zold_idx_1)) +
              std::abs(z[2] - zold_idx_2) <
          0.001) {
        exitg1 = true;
      } else {
        // 'bGLS_phase_model_single_and_multiperson:83' zold=z;
        zold_idx_0 = z[0];
        zold_idx_1 = z[1];
        zold_idx_2 = z[2];
        iter++;
      }
    }
    //  end of bGLS iterations.
    //  output variables
    // 'bGLS_phase_model_single_and_multiperson:87' alphas=-z';
    // 'bGLS_phase_model_single_and_multiperson:88' sm=sqrt(-K12);
    K12 = std::sqrt(-K12);
    // 'bGLS_phase_model_single_and_multiperson:89' st=sqrt(K11-2*(sm^2));
    stE[SUBJ] = std::sqrt(K11 - 2.0 * (K12 * K12));
    //  alphas - alpha from SUBJ to each player
    //  st     - timekeeper noise for SUBJ
    //  sm     - motor noise for SUBJ
    //  register results
    // 'bgls_from_onsets_and_window:44' alphaE(SUBJ,others) = alphas;
    alphaE[static_cast<int>(others_data[0]) - 1][SUBJ] = -z[0];
    alphaE[static_cast<int>(others_data[1]) - 1][SUBJ] = -z[1];
    alphaE[static_cast<int>(others_data[2]) - 1][SUBJ] = -z[2];
    // 'bgls_from_onsets_and_window:45' smE(SUBJ) = sm;
    smE[SUBJ] = K12;
    // 'bgls_from_onsets_and_window:46' stE(SUBJ) = st;
  }
}

// End of code generation (bgls_from_onsets_and_window.cpp)
