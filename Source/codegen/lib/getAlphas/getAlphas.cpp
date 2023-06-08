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
#include "bgls_from_onsets_and_window.h"
#include "diff.h"
#include "eml_setop.h"
#include "getAlphas_data.h"
#include "getAlphas_initialize.h"
#include "inv.h"
#include "randn.h"
#include "rt_nonfinite.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// Function Definitions
//
// function [alphaE, stE, smE] = getAlphas(vl1, vl2, vla, vlc, use_padding)
void getAlphas(const double vl1_data[], const int vl1_size[1],
               const double vl2_data[], const int vl2_size[1],
               const double vla_data[], const int vla_size[1],
               const double vlc_data[], const int vlc_size[1],
               boolean_T use_padding, double alphaE[4][4], double stE[4],
               double smE[4])
{
  static const double alphaMAT[4][4]{{0.0, 0.25, 0.25, 0.25},
                                     {0.0, 0.0, 0.0, 0.0},
                                     {0.0, 0.0, 0.0, 0.0},
                                     {0.0, 0.0, 0.0, 0.0}};
  double c_d[18][18];
  double d[18][18];
  double Am_data[304];
  double padded_wRm_data[152];
  double tmp_data[152];
  double ORm_data[80];
  double Rm_data[76];
  double others_data[4];
  double z[3];
  double K12;
  int ia_data[4];
  int Am_size[3];
  int ORm_size[2];
  int Rm_size[2];
  int b_Rm_size[2];
  int padded_wRm_size[2];
  int ar;
  int br;
  int i;
  int i1;
  int ic;
  int loop_ub_tmp;
  int padding_size;
  signed char subsb_idx_1;
  if (!isInitialized_getAlphas) {
    getAlphas_initialize();
  }
  //  % This function calculates alpha values from intput onsets. Called from
  //  the C++ plugin.
  //  NOTE: For this function vl1, etc, are given as onsets (not IOIs).
  //  This is so that asynchronies (Am) can be calculated even if not all
  //  (i.e. only the last 20) onsets are provided.
  // 'getAlphas:10' if (nargin < 5)
  //  NOTE: The padded bgls code works with onsets/IOIs provided in
  //  milliseconds.
  //  Number of onsets provided
  // 'getAlphas:17' l = size(vl1, 1);
  //  Number of intervals that can be calculated
  // 'getAlphas:19' I = l-1;
  //  Remember!!!
  //  Rm  - IOIs for each player (?)
  //  ORm - Onsets (in ms) for each player.
  //  Am  - Matrices of asynchronies between players (-ve means
  //        ahead)
  // 'getAlphas:27' ORm = zeros(l,4);
  ORm_size[0] = vl1_size[0];
  ORm_size[1] = 4;
  ar = vl1_size[0];
  for (i = 0; i < 4; i++) {
    for (i1 = 0; i1 < ar; i1++) {
      ORm_data[i1 + ORm_size[0] * i] = 0.0;
    }
  }
  // 'getAlphas:28' ORm(1:l, 1) = vl1;
  ar = vl1_size[0];
  if (ar - 1 >= 0) {
    std::copy(&vl1_data[0], &vl1_data[ar], &ORm_data[0]);
  }
  // 'getAlphas:29' ORm(1:l, 2) = vl2;
  ar = vl2_size[0];
  for (i = 0; i < ar; i++) {
    ORm_data[i + ORm_size[0]] = vl2_data[i];
  }
  // 'getAlphas:30' ORm(1:l, 3) = vla;
  ar = vla_size[0];
  for (i = 0; i < ar; i++) {
    ORm_data[i + ORm_size[0] * 2] = vla_data[i];
  }
  // 'getAlphas:31' ORm(1:l, 4) = vlc;
  ar = vlc_size[0];
  for (i = 0; i < ar; i++) {
    ORm_data[i + ORm_size[0] * 3] = vlc_data[i];
  }
  //  NOTE: Asynchrony (Am) calc is essentially identical to before,
  //  but using ORm, instead of Rm
  //  Am = cell(I, 1);
  // 'getAlphas:36' Am = zeros(I,4,4) ;
  Am_size[0] = vl1_size[0] - 1;
  Am_size[1] = 4;
  Am_size[2] = 4;
  loop_ub_tmp = vl1_size[0] - 1;
  for (i = 0; i < 4; i++) {
    for (i1 = 0; i1 < 4; i1++) {
      for (ic = 0; ic < loop_ub_tmp; ic++) {
        Am_data[(ic + Am_size[0] * i1) + Am_size[0] * 4 * i] = 0.0;
      }
    }
  }
  // 'getAlphas:37' for k = 2:l
  i = vl1_size[0];
  for (int k{0}; k <= i - 2; k++) {
    // 'getAlphas:38' j = k-1;
    //  Am{j, :, :) = zeros(4, 4);
    // 'getAlphas:40' for a = 1:4
    i1 = ORm_size[0];
    ic = Am_size[0];
    for (ar = 0; ar < 4; ar++) {
      // 'getAlphas:41' for b = 1:4
      // 'getAlphas:42' ORm_a = ORm(k, a);
      // 'getAlphas:43' ORm_b = ORm(k, b);
      // 'getAlphas:44' Am(j, a, b) = ORm_a - ORm_b;
      K12 = ORm_data[(k + i1 * ar) + 1];
      br = k + ic * ar;
      Am_data[br] = K12 - ORm_data[k + 1];
      // 'getAlphas:42' ORm_a = ORm(k, a);
      // 'getAlphas:43' ORm_b = ORm(k, b);
      // 'getAlphas:44' Am(j, a, b) = ORm_a - ORm_b;
      Am_data[br + ic * 4] = K12 - ORm_data[(k + i1) + 1];
      // 'getAlphas:42' ORm_a = ORm(k, a);
      // 'getAlphas:43' ORm_b = ORm(k, b);
      // 'getAlphas:44' Am(j, a, b) = ORm_a - ORm_b;
      Am_data[br + ic * 4 * 2] = K12 - ORm_data[(k + i1 * 2) + 1];
      // 'getAlphas:42' ORm_a = ORm(k, a);
      // 'getAlphas:43' ORm_b = ORm(k, b);
      // 'getAlphas:44' Am(j, a, b) = ORm_a - ORm_b;
      Am_data[br + ic * 4 * 3] = K12 - ORm_data[(k + i1 * 3) + 1];
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
  // 'getAlphas:58' Rm = diff(ORm,1);
  coder::diff(ORm_data, ORm_size, padded_wRm_data, padded_wRm_size);
  Rm_size[0] = padded_wRm_size[0];
  Rm_size[1] = 4;
  ar = padded_wRm_size[0];
  for (i = 0; i < 4; i++) {
    for (i1 = 0; i1 < ar; i1++) {
      Rm_data[i1 + Rm_size[0] * i] =
          padded_wRm_data[i1 + padded_wRm_size[0] * i];
    }
  }
  //  NOTE: New calculation sets W_SIZE to 20 as a constant
  // 'getAlphas:61' W_SIZE = 19 ;
  // 'getAlphas:63' ORm = ORm(2:l,:) ;
  if (vl1_size[0] < 2) {
    i = -1;
    i1 = -1;
  } else {
    i = 0;
    i1 = vl1_size[0] - 1;
  }
  loop_ub_tmp = i1 - i;
  for (i1 = 0; i1 < 4; i1++) {
    for (ic = 0; ic < loop_ub_tmp; ic++) {
      ORm_data[ic + loop_ub_tmp * i1] =
          ORm_data[((i + ic) + ORm_size[0] * i1) + 1];
    }
  }
  //  Am = Ammat2cell(Am);
  // 'getAlphas:67' [alphaE,stE,smE] = bgls_padded(Rm, Am, ORm, I,
  // W_SIZE,use_padding);
  //   Rm, Am, and ORm as specified elsewhere
  //   I -> the index being sampled.
  //   W_SIZE -> the window size to use (i.e. how far into the past do we
  //   look?)
  //   NOTE: "I" was previously "i". This was changed for consistency with
  //   bgls sub_functions, and to prevent potential future conflicts with
  //   commonly used "i" loop variable.
  // 'bgls_padded:11' if nargin < 6
  //  NOTE: Tempo is currently constant at 120bpm (or 500ms). This can/should
  //  be added as an input variable from the plugin.
  // 'bgls_padded:17' tempo = 500;
  //  This could be more sophisticated...
  //  NOTE: S_SIZE parameter is currently always the same as I, since
  //  I is calculated as size(vl1,1) in parent function.
  // 'bgls_padded:21' S_SIZE = size(Rm, 1);
  //   Get padding based on currently observed IOIs
  //   Calculate the amount of padding required
  //   NOTE: W_SIZE is set at 20 in parent function. So the input values
  //   are always padded until there are at least 20 onsets/intervals
  // 'bgls_padded:29' if use_padding
  if (use_padding) {
    // 'bgls_padded:30' padding_size = max(W_SIZE - I, 0);
    padding_size = 19 - vl1_size[0];
  } else {
    // 'bgls_padded:31' else
    // 'bgls_padded:32' padding_size = 0;
    padding_size = -1;
  }
  // 'bgls_padded:35' if padding_size == 0
  if (padding_size + 1 == 0) {
    //   If we don't need padding, just follow the 'naive'
    //   approach
    // 'bgls_padded:38' [alphaE, stE, smE] = bgls_from_onsets_and_window(Rm, Am,
    // ORm, I, W_SIZE);
    bgls_from_onsets_and_window(Rm_data, Rm_size, Am_data, Am_size,
                                static_cast<double>(vl1_size[0]) - 1.0, alphaE,
                                stE, smE);
  } else {
    double padded_wAm_data[608];
    double pAm[4][4][19];
    double c_Rm_data[156];
    double Mn[4][20];
    double dv[4][20];
    double Tn[4][19];
    double b_Rm_data[76];
    double pORm[4][19];
    double pRm[4][19];
    double K[2][2];
    double Rm;
    int vl1_idx_0_tmp;
    //  NOTE: Initial Alpha and sigma values should be consistent with the
    //  values used in the C++ plugin.
    //  These declarations have been moved down, since they are unnecessary
    //  when no padding is required.
    //   Simulation parameters for padding
    // p_alphaMAT = [0.00, 0.25, 0.25, 0.25;
    //               0.25, 0.00, 0.25, 0.25;
    //               0.25, 0.25, 0.00, 0.25;
    //               0.25, 0.25, 0.25, 0.00];
    // 'bgls_padded:54' p_alphaMAT = [0.00, 0.00, 0.00, 0.00;
    // 'bgls_padded:55'                   0.25, 0.00, 0.00, 0.00;
    // 'bgls_padded:56'                   0.25, 0.00, 0.00, 0.00;
    // 'bgls_padded:57'                   0.25, 0.00, 0.00, 0.00];
    // 'bgls_padded:58' p_sigmaMs = [6,6,6,6];
    // 'bgls_padded:59' p_sigmaTs = [14,14,14,14];
    //   Create matrices to hold the regular Rm and Am PLUS padding.
    // 'bgls_padded:63' padded_wRm = nan(S_SIZE + padding_size, 4);
    vl1_idx_0_tmp = (padded_wRm_size[0] + padding_size) + 1;
    // 'bgls_padded:64' padded_wAm = zeros(S_SIZE + padding_size, 4, 4);
    //   Simulate padding with Nori's code
    // 'bgls_padded:67' [pRm, pAm, pORm] =
    // Simulate_phase_correction_ensemble(tempo, W_SIZE, ... 'bgls_padded:68'
    // p_alphaMAT, p_sigmaTs, p_sigmaMs);
    // %%%%%%%%%%%%%%%%%%%%%%
    //  This function compute a simulation of phase correcting ensemble (fixed
    //  tempo) of synchornizers, using the generalization of the Vorberg and
    //  Schulze (2002) to ensemble of synchornizers
    //
    //    For more information see: Nori Jacoby, Naftali Tishby, Bruno H. Repp,
    //    Merav Ahissar and Peter E. Keller (2015)
    //
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
    // 'Simulate_phase_correction_ensemble:24' P=length(sigmaMs);
    // 'Simulate_phase_correction_ensemble:25' assert(size(sigmaTs,2)==P);
    // 'Simulate_phase_correction_ensemble:26' assert(size(sigmaMs,2)==P);
    // 'Simulate_phase_correction_ensemble:27' assert(size(sigmaTs,1)==1);
    // 'Simulate_phase_correction_ensemble:28' assert(size(sigmaMs,1)==1);
    // 'Simulate_phase_correction_ensemble:30' assert(size(alphaMAT,1)==P);
    // 'Simulate_phase_correction_ensemble:31' assert(size(alphaMAT,2)==P);
    // 'Simulate_phase_correction_ensemble:32' Tn=repmat(sigmaTs,[N 1]) .*
    // randn(N,P);
    for (int k{0}; k < 4; k++) {
      for (i = 0; i < vl1_idx_0_tmp; i++) {
        padded_wRm_data[i + vl1_idx_0_tmp * k] = rtNaN;
      }
      for (i = 0; i < 4; i++) {
        for (i1 = 0; i1 < vl1_idx_0_tmp; i1++) {
          padded_wAm_data[(i1 + vl1_idx_0_tmp * i) + vl1_idx_0_tmp * 4 * k] =
              0.0;
        }
      }
      for (br = 0; br < 19; br++) {
        Tn[k][br] = 14.0;
      }
    }
    coder::randn(pORm);
    // 'Simulate_phase_correction_ensemble:33' Mn=repmat(sigmaMs,[N+1 1]) .*
    // randn(N+1,P);
    for (int k{0}; k < 4; k++) {
      for (i = 0; i < 19; i++) {
        Tn[k][i] *= pORm[k][i];
      }
      for (br = 0; br < 20; br++) {
        Mn[k][br] = 6.0;
      }
    }
    coder::b_randn(dv);
    // 'Simulate_phase_correction_ensemble:34' Hn=Tn(:,1:P)+ Mn(2:end,1:P)-
    // Mn(1:(end-1),1:P) ; 'Simulate_phase_correction_ensemble:36'
    // ORm=zeros(N,P); 'Simulate_phase_correction_ensemble:37' Rm=zeros(N,P);
    //  Am=cell(N,1);
    // 'Simulate_phase_correction_ensemble:39' Am = zeros(N, P, P);
    // 'Simulate_phase_correction_ensemble:41'
    // ORm(1:2,1:P)=repmat([0;init_tempo],[1 P]);
    // 'Simulate_phase_correction_ensemble:42'
    // Rm(1:2,1:P)=repmat([init_tempo;init_tempo],[1 P]);
    for (br = 0; br < 4; br++) {
      for (i = 0; i < 20; i++) {
        Mn[br][i] *= dv[br][i];
      }
      for (i = 0; i < 19; i++) {
        Tn[br][i] = (Tn[br][i] + Mn[br][i + 1]) - Mn[br][i];
        pORm[br][i] = 0.0;
        pRm[br][i] = 0.0;
      }
      for (i = 0; i < 4; i++) {
        for (i1 = 0; i1 < 19; i1++) {
          pAm[br][i][i1] = 0.0;
        }
      }
      pORm[br][0] = 0.0;
      pRm[br][0] = 500.0;
      pORm[br][1] = 500.0;
      pRm[br][1] = 500.0;
    }
    //  Am{1}=zeros(P,P);
    // 'Simulate_phase_correction_ensemble:45' for K=1:2
    for (ar = 0; ar < 2; ar++) {
      // 'Simulate_phase_correction_ensemble:46' for I=1:P
      for (br = 0; br < 4; br++) {
        // 'Simulate_phase_correction_ensemble:47' for J=1:P
        // 'Simulate_phase_correction_ensemble:48' Am(K,I,J)=ORm(K,I)-ORm(K,J);
        pAm[0][br][ar] = pORm[br][ar] - pORm[0][ar];
        // 'Simulate_phase_correction_ensemble:48' Am(K,I,J)=ORm(K,I)-ORm(K,J);
        pAm[1][br][ar] = pORm[br][ar] - pORm[1][ar];
        // 'Simulate_phase_correction_ensemble:48' Am(K,I,J)=ORm(K,I)-ORm(K,J);
        pAm[2][br][ar] = pORm[br][ar] - pORm[2][ar];
        // 'Simulate_phase_correction_ensemble:48' Am(K,I,J)=ORm(K,I)-ORm(K,J);
        pAm[3][br][ar] = pORm[br][ar] - pORm[3][ar];
      }
    }
    // 'Simulate_phase_correction_ensemble:54' for K=(1):(N-1)
    for (ar = 0; ar < 18; ar++) {
      // 'Simulate_phase_correction_ensemble:55' for I=1:P
      for (br = 0; br < 4; br++) {
        // 'Simulate_phase_correction_ensemble:56' ORm(K+1,I)=ORm(K,I)+Hn(K,I) +
        // sum ((-alphaMAT(I,:).*squeeze(Am(K,I,:))'))+init_tempo;
        K[0][0] = -alphaMAT[0][br] * pAm[0][br][ar];
        K[0][1] = -alphaMAT[1][br] * pAm[1][br][ar];
        K[1][0] = -alphaMAT[2][br] * pAm[2][br][ar];
        K[1][1] = -alphaMAT[3][br] * pAm[3][br][ar];
        Rm = pORm[br][ar];
        K12 = ((Rm + Tn[br][ar]) +
               ((((&K[0][0])[0] + (&K[0][0])[1]) + (&K[0][0])[2]) +
                (&K[0][0])[3])) +
              500.0;
        pORm[br][ar + 1] = K12;
        // 'Simulate_phase_correction_ensemble:57' Rm(K+1,I)=ORm(K+1,I)-
        // ORm(K,I);
        pRm[br][ar + 1] = K12 - Rm;
      }
      // 'Simulate_phase_correction_ensemble:60' for I=1:P
      for (br = 0; br < 4; br++) {
        // 'Simulate_phase_correction_ensemble:61' for J=1:P
        // 'Simulate_phase_correction_ensemble:62'
        // Am(K+1,I,J)=ORm(K+1,I)-ORm(K+1,J);
        K12 = pORm[br][ar + 1];
        pAm[0][br][ar + 1] = K12 - pORm[0][ar + 1];
        // 'Simulate_phase_correction_ensemble:62'
        // Am(K+1,I,J)=ORm(K+1,I)-ORm(K+1,J);
        pAm[1][br][ar + 1] = K12 - pORm[1][ar + 1];
        // 'Simulate_phase_correction_ensemble:62'
        // Am(K+1,I,J)=ORm(K+1,I)-ORm(K+1,J);
        pAm[2][br][ar + 1] = K12 - pORm[2][ar + 1];
        // 'Simulate_phase_correction_ensemble:62'
        // Am(K+1,I,J)=ORm(K+1,I)-ORm(K+1,J);
        pAm[3][br][ar + 1] = K12 - pORm[3][ar + 1];
      }
    }
    //  CELL
    //  pAm = Amcell2mat(pAm);
    //  if W_SIZE - I == 1
    //   In this case, the simulation code is a bit weird and gives back
    //   2 x Y matrices instead of a 1 x Y matrices, so just take the first
    //   row of each!
    // 'bgls_padded:77' pRmPad = pRm(1:padding_size, :);
    // 'bgls_padded:78' pAmPad = pAm(1:padding_size, : ,:);
    // 'bgls_padded:79' pORmPad = pORm(1:padding_size, :);
    //  end
    // 'bgls_padded:82' padded_wRm(1:padding_size, :) = pRmPad;
    for (i = 0; i < 4; i++) {
      for (i1 = 0; i1 <= padding_size; i1++) {
        padded_wRm_data[i1 + vl1_idx_0_tmp * i] = pRm[i][i1];
      }
    }
    // 'bgls_padded:83' padded_wRm(padding_size + 1:end, :) = Rm;
    if (padding_size + 2 > vl1_idx_0_tmp) {
      i = 0;
    } else {
      i = padding_size + 1;
    }
    // 'bgls_padded:85' padded_wAm(1:padding_size, :, :) = pAmPad;
    ar = padded_wRm_size[0];
    for (i1 = 0; i1 < 4; i1++) {
      for (ic = 0; ic < ar; ic++) {
        padded_wRm_data[(i + ic) + vl1_idx_0_tmp * i1] =
            Rm_data[ic + Rm_size[0] * i1];
      }
      for (ic = 0; ic < 4; ic++) {
        for (br = 0; br <= padding_size; br++) {
          padded_wAm_data[(br + vl1_idx_0_tmp * ic) + vl1_idx_0_tmp * 4 * i1] =
              pAm[i1][ic][br];
        }
      }
    }
    // 'bgls_padded:86' padded_wAm(padding_size + 1:end, :, :) = Am;
    if (padding_size + 2 > vl1_idx_0_tmp) {
      i = 0;
    } else {
      i = padding_size + 1;
    }
    // 'bgls_padded:88' [padded_wORm, padded_wRm] = recalculateORm(ORm,
    // padded_wRm, padding_size, pRm(padding_size+1,1)); 'recalculateORm:5'
    // ORmPad = cumsum(Rm(1:nPad,:),1)-Rm(1,:);
    ar = vl1_size[0] - 1;
    for (i1 = 0; i1 < 4; i1++) {
      for (ic = 0; ic < 4; ic++) {
        for (br = 0; br < ar; br++) {
          padded_wAm_data[((i + br) + vl1_idx_0_tmp * ic) +
                          vl1_idx_0_tmp * 4 * i1] =
              Am_data[(br + Am_size[0] * ic) + Am_size[0] * 4 * i1];
        }
      }
      for (ic = 0; ic <= padding_size; ic++) {
        Rm_data[ic + (padding_size + 1) * i1] =
            padded_wRm_data[ic + vl1_idx_0_tmp * i1];
      }
    }
    i = padding_size + 1;
    for (int k{0}; k < 4; k++) {
      if (padding_size - 1 >= 0) {
        subsb_idx_1 = static_cast<signed char>(k + 1);
      }
      for (ar = 0; ar <= i - 2; ar++) {
        br = (ar + (padding_size + 1) * (subsb_idx_1 - 1)) + 1;
        Rm_data[br] += Rm_data[ar + (padding_size + 1) * k];
      }
    }
    br = padding_size + 1;
    ar = padding_size + 1;
    for (i = 0; i < 4; i++) {
      for (i1 = 0; i1 < ar; i1++) {
        b_Rm_data[i1 + (padding_size + 1) * i] =
            Rm_data[i1 + (padding_size + 1) * i] -
            padded_wRm_data[vl1_idx_0_tmp * i];
      }
    }
    for (i = 0; i < 4; i++) {
      for (i1 = 0; i1 < br; i1++) {
        Rm_data[i1 + (padding_size + 1) * i] =
            b_Rm_data[i1 + (padding_size + 1) * i];
      }
    }
    // 'recalculateORm:6' ORm = ORm-ORm(1,1)+tempo_fill+ORmPad(nPad,1);
    K12 = ORm_data[0];
    Rm = Rm_data[padding_size];
    for (i = 0; i < 4; i++) {
      for (i1 = 0; i1 < loop_ub_tmp; i1++) {
        ar = i1 + loop_ub_tmp * i;
        ORm_data[ar] = ((ORm_data[ar] - K12) + pRm[0][padding_size + 1]) + Rm;
      }
    }
    // 'recalculateORm:8' ORmNew = [ORmPad;ORm];
    // 'recalculateORm:10' RmNew = Rm;
    // 'recalculateORm:11' RmNew(2:end,:) = diff(ORmNew,1) ;
    i = (vl1_idx_0_tmp >= 2);
    b_Rm_size[0] = (padding_size + loop_ub_tmp) + 1;
    b_Rm_size[1] = 4;
    ar = padding_size + 1;
    for (i1 = 0; i1 < 4; i1++) {
      for (ic = 0; ic < ar; ic++) {
        c_Rm_data[ic + b_Rm_size[0] * i1] =
            Rm_data[ic + (padding_size + 1) * i1];
      }
      for (ic = 0; ic < loop_ub_tmp; ic++) {
        c_Rm_data[((ic + padding_size) + b_Rm_size[0] * i1) + 1] =
            ORm_data[ic + loop_ub_tmp * i1];
      }
    }
    coder::diff(c_Rm_data, b_Rm_size, tmp_data, ORm_size);
    //  [padded_wORm, padded_wRm] = recalculateORm(ORm, padded_wRm,
    //  padding_size, 500); AmMat = Amcell2mat(padded_wAm); Am1 =
    //  squeeze(AmMat(:,1,:)); Am2 = padded_wORm - padded_wORm(:,1);
    //   Estimate with 'full' window
    //  padded_wAm = Ammat2cell(padded_wAm);
    // 'bgls_padded:97' [alphaE,stE,smE] =
    // bgls_specified_window(padded_wRm(1:W_SIZE, :), padded_wAm(1:W_SIZE,:,:));
    //  Initialise trial estimates
    // 'bgls_specified_window:4' alphaE = zeros(4,4);
    ar = ORm_size[0];
    for (i1 = 0; i1 < 4; i1++) {
      for (ic = 0; ic < ar; ic++) {
        padded_wRm_data[(i + ic) + vl1_idx_0_tmp * i1] =
            tmp_data[ic + ORm_size[0] * i1];
      }
      for (ic = 0; ic < 19; ic++) {
        pORm[i1][ic] = padded_wRm_data[ic + vl1_idx_0_tmp * i1];
      }
      alphaE[i1][0] = 0.0;
      alphaE[i1][1] = 0.0;
      alphaE[i1][2] = 0.0;
      alphaE[i1][3] = 0.0;
    }
    // 'bgls_specified_window:5' smE = zeros(4,1);
    // 'bgls_specified_window:6' stE = zeros(4,1);
    // 'bgls_specified_window:8' for SUBJ=1:4
    for (int SUBJ{0}; SUBJ < 4; SUBJ++) {
      double As[3][19];
      double b3[18];
      double MEAN_A[3];
      double K11;
      boolean_T exitg1;
      //   prepare dataset
      // 'bgls_specified_window:11' R = wRm(:,SUBJ);
      //  R is the IOIs for SUBJ (NB: first row is 170s b/c of 'zero point'
      //  where each onset starts after the tempo in ms)
      // 'bgls_specified_window:12' As = zeros(size(R,1),3);
      //  size(R, 1) is the number of rows in R,
      // 'bgls_specified_window:13' others = setdiff(1:4,SUBJ);
      coder::do_vectors(static_cast<double>(SUBJ) + 1.0, others_data, ORm_size,
                        ia_data, ar);
      //  numbers that aren't the current SUBJ index
      // 'bgls_specified_window:14' assert(length(others) == 3);
      // 'bgls_specified_window:15' for K=1:length(wAm)
      Rm = others_data[0];
      K12 = others_data[1];
      K11 = others_data[2];
      for (ar = 0; ar < 19; ar++) {
        //  Iterates through Am, which is the matrix of asynchronies at each
        //  point
        // 'bgls_specified_window:16' As(K,1) = wAm(K,SUBJ,others(1));
        br = ar + vl1_idx_0_tmp * SUBJ;
        As[0][ar] = padded_wAm_data[br + vl1_idx_0_tmp * 4 *
                                             (static_cast<int>(Rm) - 1)];
        //  As is the asynchronies between SUBJ and the other three players (Ks)
        // 'bgls_specified_window:17' As(K,2) = wAm(K,SUBJ,others(2));
        As[1][ar] = padded_wAm_data[br + vl1_idx_0_tmp * 4 *
                                             (static_cast<int>(K12) - 1)];
        //  Thus you get a (#players - 1) x N matrix
        // 'bgls_specified_window:18' As(K,3) = wAm(K,SUBJ,others(3));
        As[2][ar] = padded_wAm_data[br + vl1_idx_0_tmp * 4 *
                                             (static_cast<int>(K11) - 1)];
      }
      //  do bGLS for each player
      // 'bgls_specified_window:22' MEAN_A = mean(As);
      for (int k{0}; k < 3; k++) {
        Rm = As[k][0];
        for (ar = 0; ar < 18; ar++) {
          Rm += As[k][ar + 1];
        }
        MEAN_A[k] = Rm;
      }
      MEAN_A[0] /= 19.0;
      MEAN_A[1] /= 19.0;
      MEAN_A[2] /= 19.0;
      //  Calculate the average asynchrony for SUBJ
      // 'bgls_specified_window:23' MEAN_R = mean(R);
      K12 = pORm[SUBJ][0];
      for (int k{0}; k < 18; k++) {
        K12 += pORm[SUBJ][k + 1];
      }
      K12 /= 19.0;
      //  Calculate the average IOI for SUBJ
      // 'bgls_specified_window:24' [alphas,st,sm] =
      // bGLS_phase_model_single_and_multiperson(R,As,MEAN_A,MEAN_R);
      // %%%%%%%%%%%%%%%%%%
      //  This method computes the bGLS for single and multiple subject.
      //  it is the function to use when there are no signficant tempo cahnges.
      //
      //    For more information see: Nori Jacoby, Naftali Tishby, Bruno H.
      //    Repp, Merav Ahissar and Peter E. Keller (2015)
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
      for (br = 0; br < 3; br++) {
        // 'bGLS_phase_model_single_and_multiperson:47'
        // As(:,p)=As(:,p)-MEAN_A(p);
        for (i = 0; i < 19; i++) {
          As[br][i] -= MEAN_A[br];
        }
      }
      //  compute matrices
      // 'bGLS_phase_model_single_and_multiperson:51' b3=R(2:end)-MEAN_R;
      for (i = 0; i < 18; i++) {
        b3[i] = pORm[SUBJ][i + 1] - K12;
      }
      // 'bGLS_phase_model_single_and_multiperson:52' A3=[As(1:(end-1),:)];
      //  init acvf
      // 'bGLS_phase_model_single_and_multiperson:55' K11=1;
      K11 = 1.0;
      // 'bGLS_phase_model_single_and_multiperson:56' K12=0;
      K12 = 0.0;
      // 'bGLS_phase_model_single_and_multiperson:58' zold=zeros(P,1)-9999;
      MEAN_A[0] = -9999.0;
      MEAN_A[1] = -9999.0;
      MEAN_A[2] = -9999.0;
      // init to invalid value
      //  do the BGLS iterations
      // 'bGLS_phase_model_single_and_multiperson:61' for iter=1:ITER
      loop_ub_tmp = 0;
      exitg1 = false;
      while ((!exitg1) && (loop_ub_tmp < 20)) {
        double b_d[18][18];
        double y_tmp[18][3];
        double x[2][17];
        double d_d[18];
        double b_y_tmp[3][3];
        double dv1[3][3];
        // 'bGLS_phase_model_single_and_multiperson:62'
        // CC=diag(K11*ones(1,N),0)+ diag(K12*ones(1,N-1),1) +
        // diag(K12*ones(1,N-1),-1);
        std::memset(&d[0][0], 0, 324U * sizeof(double));
        for (ar = 0; ar < 18; ar++) {
          d[ar][ar] = K11;
          for (i = 0; i < 18; i++) {
            b_d[ar][i] = 0.0;
          }
        }
        for (ar = 0; ar < 17; ar++) {
          b_d[ar + 1][ar] = K12;
        }
        std::memset(&c_d[0][0], 0, 324U * sizeof(double));
        for (ar = 0; ar < 17; ar++) {
          c_d[ar][ar + 1] = K12;
        }
        // 'bGLS_phase_model_single_and_multiperson:63' iC=inv(CC);
        // 'bGLS_phase_model_single_and_multiperson:64'
        // z=inv((A3')*iC*A3)*((A3')*iC*b3);
        for (i = 0; i < 18; i++) {
          for (i1 = 0; i1 < 18; i1++) {
            b_d[i][i1] = (d[i][i1] + b_d[i][i1]) + c_d[i][i1];
          }
        }
        coder::b_inv(b_d, d);
        for (i = 0; i < 3; i++) {
          for (i1 = 0; i1 < 18; i1++) {
            Rm = 0.0;
            for (ic = 0; ic < 18; ic++) {
              Rm += As[i][ic] * d[i1][ic];
            }
            y_tmp[i1][i] = Rm;
          }
          for (i1 = 0; i1 < 3; i1++) {
            Rm = 0.0;
            for (ic = 0; ic < 18; ic++) {
              Rm += y_tmp[ic][i] * As[i1][ic];
            }
            b_y_tmp[i1][i] = Rm;
          }
          Rm = 0.0;
          for (i1 = 0; i1 < 18; i1++) {
            Rm += y_tmp[i1][i] * b3[i1];
          }
          z[i] = Rm;
        }
        coder::inv(b_y_tmp, dv1);
        Rm = z[0];
        K12 = z[1];
        K11 = z[2];
        for (i = 0; i < 3; i++) {
          z[i] = (dv1[0][i] * Rm + dv1[1][i] * K12) + dv1[2][i] * K11;
        }
        //  compute GLS
        // 'bGLS_phase_model_single_and_multiperson:65' d=A3*z-b3;
        Rm = z[0];
        K12 = z[1];
        K11 = z[2];
        for (i = 0; i < 18; i++) {
          d_d[i] = ((As[0][i] * Rm + As[1][i] * K12) + As[2][i] * K11) - b3[i];
        }
        //  compute residual noise
        // 'bGLS_phase_model_single_and_multiperson:67'
        // K=cov(d(1:(end-1)),d(2:end));
        for (i = 0; i < 17; i++) {
          x[0][i] = d_d[i];
          x[1][i] = d_d[i + 1];
        }
        for (ar = 0; ar < 2; ar++) {
          K12 = 0.0;
          for (br = 0; br < 17; br++) {
            K12 += x[ar][br];
          }
          K12 /= 17.0;
          for (br = 0; br < 17; br++) {
            x[ar][br] -= K12;
          }
        }
        for (padding_size = 0; padding_size <= 2; padding_size += 2) {
          i = padding_size + 1;
          i1 = padding_size + 2;
          for (ic = i; ic <= i1; ic++) {
            (&K[0][0])[ic - 1] = 0.0;
          }
        }
        br = -1;
        for (padding_size = 0; padding_size <= 2; padding_size += 2) {
          ar = -1;
          i = padding_size + 1;
          i1 = padding_size + 2;
          for (ic = i; ic <= i1; ic++) {
            K12 = 0.0;
            for (int k{0}; k < 17; k++) {
              K12 += (&x[0][0])[(k + ar) + 1] * (&x[0][0])[(k + br) + 1];
            }
            (&K[0][0])[ic - 1] += 0.0625 * K12;
            ar += 17;
          }
          br += 17;
        }
        // estimate residual acvf
        // 'bGLS_phase_model_single_and_multiperson:68' K11=(K(1,1)+K(2,2))/2;
        K11 = (K[0][0] + K[1][1]) / 2.0;
        // 'bGLS_phase_model_single_and_multiperson:69' K12=K(1,2);
        K12 = K[1][0];
        //  apply bound
        // 'bGLS_phase_model_single_and_multiperson:72' if K12>0
        if (K[1][0] > 0.0) {
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
        if ((std::abs(z[0] - MEAN_A[0]) + std::abs(z[1] - MEAN_A[1])) +
                std::abs(z[2] - MEAN_A[2]) <
            0.001) {
          exitg1 = true;
        } else {
          // 'bGLS_phase_model_single_and_multiperson:83' zold=z;
          MEAN_A[0] = z[0];
          MEAN_A[1] = z[1];
          MEAN_A[2] = z[2];
          loop_ub_tmp++;
        }
      }
      //  end of bGLS iterations.
      //  output variables
      // 'bGLS_phase_model_single_and_multiperson:87' alphas=-z';
      // 'bGLS_phase_model_single_and_multiperson:88' sm=sqrt(-K12);
      Rm = std::sqrt(-K12);
      // 'bGLS_phase_model_single_and_multiperson:89' st=sqrt(K11-2*(sm^2));
      stE[SUBJ] = std::sqrt(K11 - 2.0 * (Rm * Rm));
      //  alphas - alpha from SUBJ to each player
      //  st     - timekeeper noise for SUBJ
      //  sm     - motor noise for SUBJ
      //  register results
      // 'bgls_specified_window:30' alphaE(SUBJ,others) = alphas;
      alphaE[static_cast<int>(others_data[0]) - 1][SUBJ] = -z[0];
      alphaE[static_cast<int>(others_data[1]) - 1][SUBJ] = -z[1];
      alphaE[static_cast<int>(others_data[2]) - 1][SUBJ] = -z[2];
      // 'bgls_specified_window:31' smE(SUBJ) = sm;
      smE[SUBJ] = Rm;
      // 'bgls_specified_window:32' stE(SUBJ) = st;
    }
  }
}

// End of code generation (getAlphas.cpp)
