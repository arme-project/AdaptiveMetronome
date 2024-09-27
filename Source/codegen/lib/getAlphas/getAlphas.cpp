//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// getAlphas.cpp
//
// Code generation for function 'getAlphas'
//

// Include files
#include "getAlphas.h"
#include "eml_setop.h"
#include "xgetrf.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// Function Definitions
void getAlphas(const double vl1[47], const double vl2[47], const double vla[47],
               const double vlc[47], double alphaE[4][4], double stE[4],
               double smE[4])
{
  double CC[45][45];
  double b_d[45][45];
  double c_d[45][45];
  double Am[4][4][46];
  double ORm[4][47];
  double Rm[4][46];
  double b3[45];
  double ib3[45];
  double others_data[4];
  double tmp2;
  double work;
  double z_idx_0;
  double z_idx_1;
  double z_idx_2;
  int ia_data[4];
  int ixLead;
  int iyStart;
  int jBcol;
  int kAcol;
  //  % This function calculates alpha values from intput onsets. Called from
  //  the C++ plugin.
  //  NOTE: For this function vl1, etc, are given as onsets (not IOIs).
  //  This is so that asynchronies (Am) can be calculated even if not all
  //  (i.e. only the last 20) onsets are provided.
  //  if (nargin < 5)
  //      use_padding = true;
  //  end
  //  NOTE: The padded bgls code works with onsets/IOIs provided in
  //  milliseconds.
  //  Number of onsets provided
  //  Number of intervals that can be calculated
  //  Remember!!!
  //  Rm  - IOIs for each player (?)
  //  ORm - Onsets (in ms) for each player.
  //  Am  - Matrices of asynchronies between players (-ve means
  //        ahead)
  std::copy(&vl1[0], &vl1[47], &ORm[0][0]);
  std::copy(&vl2[0], &vl2[47], &ORm[1][0]);
  std::copy(&vla[0], &vla[47], &ORm[2][0]);
  std::copy(&vlc[0], &vlc[47], &ORm[3][0]);
  //  NOTE: Asynchrony (Am) calc is essentially identical to before,
  //  but using ORm, instead of Rm
  //  Am = cell(I, 1);
  for (int k{0}; k < 46; k++) {
    //  Am{j, :, :) = zeros(4, 4);
    for (iyStart = 0; iyStart < 4; iyStart++) {
      work = ORm[iyStart][k + 1];
      Am[0][iyStart][k] = work - ORm[0][k + 1];
      Am[1][iyStart][k] = work - ORm[1][k + 1];
      Am[2][iyStart][k] = work - ORm[2][k + 1];
      Am[3][iyStart][k] = work - ORm[3][k + 1];
    }
  }
  //  % IOIs (Rm) are calculated from ORm as difference between each
  //  consecutive onset.
  //  ORm == cumsum(Rm,1)
  //  Rm == diff(ORm,1)
  //  TODO: Currently diff inherently produces 1 fewer rows than there are in
  //  ORm. Pad the beginning of Rm with "tempo", to be consistent with
  //  simulated data. However a better solution to this would be ideal.
  //  tempo = 500 ;
  //  Rm = (I,4);
  iyStart = 0;
  //  NOTE: New calculation sets W_SIZE to 20 as a constant
  //  Am = Ammat2cell(Am);
  //   Rm, Am, and ORm as specified elsewhere
  //   I -> the index being sampled.
  //   W_SIZE -> the window size to use (i.e. how far into the past do we
  //   look?)
  //   NOTE: "I" was previously "i". This was changed for consistency with
  //   bgls sub_functions, and to prevent potential future conflicts with
  //   commonly used "i" loop variable.
  //  if nargin < 6
  //      use_padding = true;
  //  end
  //  NOTE: Tempo is currently constant at 120bpm (or 500ms). This can/should
  //  be added as an input variable from the plugin.
  //  This could be more sophisticated...
  //  NOTE: S_SIZE parameter is currently always the same as I, since
  //  I is calculated as size(vl1,1) in parent function.
  //   Get padding based on currently observed IOIs
  //   Calculate the amount of padding required
  //   NOTE: W_SIZE is set at 20 in parent function. So the input values
  //   are always padded until there are at least 20 onsets/intervals
  //
  //  if use_padding
  //      padding_size = max(W_SIZE - I, 0);
  //  else
  //      padding_size = 0;
  //  end
  //
  //  if padding_size == 0
  //   If we don't need padding, just follow the 'naive'
  //   approach
  //  Initialise trial estimates
  for (jBcol = 0; jBcol < 4; jBcol++) {
    ixLead = jBcol * 47 + 1;
    work = (&ORm[0][0])[jBcol * 47];
    for (kAcol = 0; kAcol < 46; kAcol++) {
      tmp2 = work;
      work = (&ORm[0][0])[ixLead + kAcol];
      (&Rm[0][0])[iyStart + kAcol] = work - tmp2;
    }
    iyStart += 46;
    alphaE[jBcol][0] = 0.0;
    alphaE[jBcol][1] = 0.0;
    alphaE[jBcol][2] = 0.0;
    alphaE[jBcol][3] = 0.0;
  }
  //  Grab the appropriate window from Rm/Am/ORm
  //  Am is just an S_SIZE x 1 matrix, so grab the appropriate
  //  rows
  //  display(max(I-W_SIZE, 1));
  //  wAm = Am(max(I-W_SIZE, 1):I,:,:);
  //  Rm is S_SIZE x 4, so grab the appropriate rows, and all 4
  //  columns
  //  Do the same for ORm
  for (int SUBJ{0}; SUBJ < 4; SUBJ++) {
    double As[3][46];
    double MEAN_A[3];
    double d;
    double zold_idx_0;
    double zold_idx_1;
    double zold_idx_2;
    int others_size[2];
    int i;
    int iter;
    boolean_T exitg1;
    //   prepare dataset
    //  R is the IOIs for SUBJ (NB: first row is 170s b/c of 'zero point' where
    //  each onset starts after the tempo in ms) size(R, 1) is the number of
    //  rows in R,
    coder::do_vectors(static_cast<double>(SUBJ) + 1.0, others_data, others_size,
                      ia_data, jBcol);
    //  numbers that aren't the current SUBJ index
    d = others_data[0];
    work = others_data[1];
    tmp2 = others_data[2];
    for (iyStart = 0; iyStart < 46; iyStart++) {
      //  Iterates through Am, which is the matrix of asynchronies at each point
      As[0][iyStart] = Am[static_cast<int>(d) - 1][SUBJ][iyStart];
      //  As is the asynchronies between SUBJ and the other three players (Ks)
      As[1][iyStart] = Am[static_cast<int>(work) - 1][SUBJ][iyStart];
      //  Thus you get a (#players - 1) x N matrix
      As[2][iyStart] = Am[static_cast<int>(tmp2) - 1][SUBJ][iyStart];
    }
    //  do bGLS for each player
    for (int k{0}; k < 3; k++) {
      d = As[k][0];
      for (iyStart = 0; iyStart < 45; iyStart++) {
        d += As[k][iyStart + 1];
      }
      MEAN_A[k] = d;
    }
    MEAN_A[0] /= 46.0;
    MEAN_A[1] /= 46.0;
    MEAN_A[2] /= 46.0;
    //  Calculate the average asynchrony for SUBJ
    work = Rm[SUBJ][0];
    for (int k{0}; k < 45; k++) {
      work += Rm[SUBJ][k + 1];
    }
    work /= 46.0;
    //  Calculate the average IOI for SUBJ
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
    //  set parameters
    // this is the maximal difference between old solution and new solution.
    //  in the iteration if we get a change smaller than TRESH we simply stop
    //  (we obtaine a local maximum).
    // number of datapoints
    // number of partners
    //  !IMPORTANT!
    //  TODO: Tom come back here and put these back after the demo!
    //  reduce mean
    for (iyStart = 0; iyStart < 3; iyStart++) {
      for (i = 0; i < 46; i++) {
        As[iyStart][i] -= MEAN_A[iyStart];
      }
    }
    //  compute matrices
    for (i = 0; i < 45; i++) {
      b3[i] = Rm[SUBJ][i + 1] - work;
    }
    //  init acvf
    tmp2 = 1.0;
    work = 0.0;
    zold_idx_0 = -9999.0;
    zold_idx_1 = -9999.0;
    zold_idx_2 = -9999.0;
    // init to invalid value
    //  do the BGLS iterations
    iter = 0;
    exitg1 = false;
    while ((!exitg1) && (iter < 20)) {
      double iA3[3][45];
      double y_tmp[45][3];
      double x[2][44];
      double y[3][3];
      double K[2][2];
      int ipiv[45];
      int i1;
      int w;
      std::memset(&CC[0][0], 0, 2025U * sizeof(double));
      for (iyStart = 0; iyStart < 45; iyStart++) {
        CC[iyStart][iyStart] = tmp2;
        for (i = 0; i < 45; i++) {
          b_d[iyStart][i] = 0.0;
        }
      }
      for (iyStart = 0; iyStart < 44; iyStart++) {
        b_d[iyStart + 1][iyStart] = work;
      }
      std::memset(&c_d[0][0], 0, 2025U * sizeof(double));
      for (iyStart = 0; iyStart < 44; iyStart++) {
        c_d[iyStart][iyStart + 1] = work;
      }
      for (i = 0; i < 45; i++) {
        for (i1 = 0; i1 < 45; i1++) {
          CC[i][i1] = (CC[i][i1] + b_d[i][i1]) + c_d[i][i1];
        }
      }
      //  iC=inv(CC);
      //  z=inv((A3')*iC*A3)*((A3')*iC*b3); % compute GLS
      //  z=((A3')*iC*A3)\((A3')*iC*b3); % compute GLS
      for (i = 0; i < 3; i++) {
        for (i1 = 0; i1 < 45; i1++) {
          iA3[i][i1] = As[i][i1];
        }
      }
      std::copy(&CC[0][0], &CC[0][0] + 2025U, &b_d[0][0]);
      coder::internal::lapack::xgetrf(b_d, ipiv);
      for (ixLead = 0; ixLead < 44; ixLead++) {
        i = ipiv[ixLead];
        if (i != ixLead + 1) {
          work = iA3[0][ixLead];
          iA3[0][ixLead] = iA3[0][i - 1];
          iA3[0][i - 1] = work;
          work = iA3[1][ixLead];
          iA3[1][ixLead] = iA3[1][i - 1];
          iA3[1][i - 1] = work;
          work = iA3[2][ixLead];
          iA3[2][ixLead] = iA3[2][i - 1];
          iA3[2][i - 1] = work;
        }
      }
      for (iyStart = 0; iyStart < 3; iyStart++) {
        jBcol = 45 * iyStart;
        for (int k{0}; k < 45; k++) {
          kAcol = 45 * k;
          i = k + jBcol;
          if ((&iA3[0][0])[i] != 0.0) {
            i1 = k + 2;
            for (ixLead = i1; ixLead < 46; ixLead++) {
              w = (ixLead + jBcol) - 1;
              (&iA3[0][0])[w] -=
                  (&iA3[0][0])[i] * (&b_d[0][0])[(ixLead + kAcol) - 1];
            }
          }
        }
      }
      for (iyStart = 0; iyStart < 3; iyStart++) {
        jBcol = 45 * iyStart;
        for (int k{44}; k >= 0; k--) {
          kAcol = 45 * k;
          i = k + jBcol;
          d = (&iA3[0][0])[i];
          if (d != 0.0) {
            (&iA3[0][0])[i] = d / (&b_d[0][0])[k + kAcol];
            for (ixLead = 0; ixLead < k; ixLead++) {
              i1 = ixLead + jBcol;
              (&iA3[0][0])[i1] -=
                  (&iA3[0][0])[i] * (&b_d[0][0])[ixLead + kAcol];
            }
          }
        }
      }
      std::copy(&b3[0], &b3[45], &ib3[0]);
      coder::internal::lapack::xgetrf(CC, ipiv);
      for (ixLead = 0; ixLead < 44; ixLead++) {
        i = ipiv[ixLead];
        if (i != ixLead + 1) {
          work = ib3[ixLead];
          ib3[ixLead] = ib3[i - 1];
          ib3[i - 1] = work;
        }
      }
      for (int k{0}; k < 45; k++) {
        kAcol = 45 * k;
        if (ib3[k] != 0.0) {
          i = k + 2;
          for (ixLead = i; ixLead < 46; ixLead++) {
            ib3[ixLead - 1] -= ib3[k] * (&CC[0][0])[(ixLead + kAcol) - 1];
          }
        }
      }
      for (int k{44}; k >= 0; k--) {
        kAcol = 45 * k;
        d = ib3[k];
        if (d != 0.0) {
          d /= (&CC[0][0])[k + kAcol];
          ib3[k] = d;
          for (ixLead = 0; ixLead < k; ixLead++) {
            ib3[ixLead] -= ib3[k] * (&CC[0][0])[ixLead + kAcol];
          }
        }
      }
      for (i = 0; i < 45; i++) {
        y_tmp[i][0] = As[0][i];
        y_tmp[i][1] = As[1][i];
        y_tmp[i][2] = As[2][i];
      }
      for (i = 0; i < 3; i++) {
        for (i1 = 0; i1 < 3; i1++) {
          d = 0.0;
          for (w = 0; w < 45; w++) {
            d += y_tmp[w][i] * iA3[i1][w];
          }
          y[i1][i] = d;
        }
        d = 0.0;
        for (i1 = 0; i1 < 45; i1++) {
          d += y_tmp[i1][i] * ib3[i1];
        }
        MEAN_A[i] = d;
      }
      iyStart = 0;
      jBcol = 1;
      ixLead = 2;
      work = std::abs(y[0][0]);
      tmp2 = std::abs(y[0][1]);
      if (tmp2 > work) {
        work = tmp2;
        iyStart = 1;
        jBcol = 0;
      }
      if (std::abs(y[0][2]) > work) {
        iyStart = 2;
        jBcol = 1;
        ixLead = 0;
      }
      y[0][jBcol] /= y[0][iyStart];
      y[0][ixLead] /= y[0][iyStart];
      y[1][jBcol] -= y[0][jBcol] * y[1][iyStart];
      y[1][ixLead] -= y[0][ixLead] * y[1][iyStart];
      y[2][jBcol] -= y[0][jBcol] * y[2][iyStart];
      y[2][ixLead] -= y[0][ixLead] * y[2][iyStart];
      if (std::abs(y[1][ixLead]) > std::abs(y[1][jBcol])) {
        kAcol = jBcol;
        jBcol = ixLead;
        ixLead = kAcol;
      }
      y[1][ixLead] /= y[1][jBcol];
      y[2][ixLead] -= y[1][ixLead] * y[2][jBcol];
      z_idx_1 = MEAN_A[jBcol] - MEAN_A[iyStart] * y[0][jBcol];
      z_idx_2 = ((MEAN_A[ixLead] - MEAN_A[iyStart] * y[0][ixLead]) -
                 z_idx_1 * y[1][ixLead]) /
                y[2][ixLead];
      z_idx_1 -= z_idx_2 * y[2][jBcol];
      z_idx_1 /= y[1][jBcol];
      z_idx_0 = ((MEAN_A[iyStart] - z_idx_2 * y[2][iyStart]) -
                 z_idx_1 * y[1][iyStart]) /
                y[0][iyStart];
      //  compute GLS
      for (i = 0; i < 45; i++) {
        ib3[i] =
            ((As[0][i] * z_idx_0 + As[1][i] * z_idx_1) + As[2][i] * z_idx_2) -
            b3[i];
      }
      //  compute residual noise
      for (i = 0; i < 44; i++) {
        x[0][i] = ib3[i];
        x[1][i] = ib3[i + 1];
      }
      for (iyStart = 0; iyStart < 2; iyStart++) {
        work = 0.0;
        for (ixLead = 0; ixLead < 44; ixLead++) {
          work += x[iyStart][ixLead];
        }
        work /= 44.0;
        for (ixLead = 0; ixLead < 44; ixLead++) {
          x[iyStart][ixLead] -= work;
        }
      }
      for (iyStart = 0; iyStart <= 2; iyStart += 2) {
        i = iyStart + 1;
        i1 = iyStart + 2;
        for (ixLead = i; ixLead <= i1; ixLead++) {
          (&K[0][0])[ixLead - 1] = 0.0;
        }
      }
      jBcol = -1;
      for (iyStart = 0; iyStart <= 2; iyStart += 2) {
        kAcol = -1;
        i = iyStart + 1;
        i1 = iyStart + 2;
        for (ixLead = i; ixLead <= i1; ixLead++) {
          work = 0.0;
          for (w = 0; w < 44; w++) {
            work += (&x[0][0])[(w + kAcol) + 1] * (&x[0][0])[(w + jBcol) + 1];
          }
          (&K[0][0])[ixLead - 1] += 0.023255813953488372 * work;
          kAcol += 44;
        }
        jBcol += 44;
      }
      // estimate residual acvf
      tmp2 = (K[0][0] + K[1][1]) / 2.0;
      work = K[1][0];
      //  apply bound
      if (K[1][0] > 0.0) {
        work = 0.0;
      }
      if (tmp2 < -3.0 * work) {
        tmp2 = -3.0 * work;
      }
      //  if allready obtain local maxima there is no point to continue...
      if ((std::abs(z_idx_0 - zold_idx_0) + std::abs(z_idx_1 - zold_idx_1)) +
              std::abs(z_idx_2 - zold_idx_2) <
          0.0001) {
        exitg1 = true;
      } else {
        zold_idx_0 = z_idx_0;
        zold_idx_1 = z_idx_1;
        zold_idx_2 = z_idx_2;
        iter++;
      }
    }
    //  end of bGLS iterations.
    //  output variables
    work = std::sqrt(-work);
    stE[SUBJ] = std::sqrt(tmp2 - 2.0 * (work * work));
    //  alphas - alpha from SUBJ to each player
    //  st     - timekeeper noise for SUBJ
    //  sm     - motor noise for SUBJ
    //  register results
    alphaE[static_cast<int>(others_data[0]) - 1][SUBJ] = -z_idx_0;
    alphaE[static_cast<int>(others_data[1]) - 1][SUBJ] = -z_idx_1;
    alphaE[static_cast<int>(others_data[2]) - 1][SUBJ] = -z_idx_2;
    smE[SUBJ] = work;
  }
  //  return
  //  end
  //  NOTE: Initial Alpha and sigma values should be consistent with the
  //  values used in the C++ plugin.
  //  These declarations have been moved down, since they are unnecessary
  //  when no padding is required.
  //   Simulation parameters for padding
  // p_alphaMAT = [0.00, 0.25, 0.25, 0.25;
  //               0.25, 0.00, 0.25, 0.25;
  //               0.25, 0.25, 0.00, 0.25;
  //               0.25, 0.25, 0.25, 0.00];
  //
  //  p_alphaMAT = [0.00, 0.00, 0.00, 0.00;
  //                0.25, 0.00, 0.00, 0.00;
  //                0.25, 0.00, 0.00, 0.00;
  //                0.25, 0.00, 0.00, 0.00];
  //  p_sigmaMs = [6,6,6,6];
  //  p_sigmaTs = [14,14,14,14];
  //
  //  %  Create matrices to hold the regular Rm and Am PLUS padding.
  //  padded_wRm = nan(S_SIZE + padding_size, 4);
  //  padded_wAm = zeros(S_SIZE + padding_size, 4, 4);
  //
  //  %  Simulate padding with Nori's code
  //  [pRm, pAm, pORm] = Simulate_phase_correction_ensemble(tempo, W_SIZE, ...
  //          p_alphaMAT, p_sigmaTs, p_sigmaMs);
  //
  //  % CELL
  //  % pAm = Amcell2mat(pAm);
  //
  //  % if W_SIZE - I == 1
  //      %  In this case, the simulation code is a bit weird and gives back
  //      %  2 x Y matrices instead of a 1 x Y matrices, so just take the first
  //      %  row of each!
  //      pRmPad = pRm(1:padding_size, :);
  //      pAmPad = pAm(1:padding_size, : ,:);
  //      pORmPad = pORm(1:padding_size, :);
  //  % end
  //
  //  padded_wRm(1:padding_size, :) = pRmPad;
  //  padded_wRm(padding_size + 1:end, :) = Rm;
  //
  //  padded_wAm(1:padding_size, :, :) = pAmPad;
  //  padded_wAm(padding_size + 1:end, :, :) = Am;
  //
  //  [padded_wORm, padded_wRm] = recalculateORm(ORm, padded_wRm, padding_size,
  //  pRm(padding_size+1,1)); % [padded_wORm, padded_wRm] = recalculateORm(ORm,
  //  padded_wRm, padding_size, 500);
  //
  //  % AmMat = Amcell2mat(padded_wAm);
  //  % Am1 = squeeze(AmMat(:,1,:));
  //  % Am2 = padded_wORm - padded_wORm(:,1);
  //
  //  %  Estimate with 'full' window
  //  % padded_wAm = Ammat2cell(padded_wAm);
  //  [alphaE,stE,smE] = bgls_specified_window(padded_wRm(1:W_SIZE, :),
  //  padded_wAm(1:W_SIZE,:,:));
}

// End of code generation (getAlphas.cpp)
