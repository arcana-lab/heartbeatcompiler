#pragma once

#include "loop_handler.hpp"
#include <cstdint>

namespace srad {

void HEARTBEAT_nest0_loop0(uint64_t rows, uint64_t cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW);
void HEARTBEAT_nest0_loop1(uint64_t cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW, uint64_t i);
void HEARTBEAT_nest1_loop0(uint64_t rows, uint64_t cols, float *J, float *dN, float *dS, float *dW, float *dE, float *c, int *iS, int *jE, float lambda);
void HEARTBEAT_nest1_loop1(uint64_t cols, float *J, float *dN, float *dS, float *dW, float *dE, float *c, int *iS, int *jE, float lambda, uint64_t i);

void srad_hb_compiler(int rows, int cols, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda) {
  HEARTBEAT_nest0_loop0(rows, cols, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW);
  HEARTBEAT_nest1_loop0(rows, cols, J, dN, dS, dW, dE, c, iS, jE, lambda);
}

// Outlined loops
// void HEARTBEAT_nest0_loop0(int rows, int cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW) {
void HEARTBEAT_nest0_loop0(uint64_t rows, uint64_t cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW) {
  // for (int i = 0 ; i < rows ; i++) {
  for (uint64_t i = 0 ; i < rows ; i++) {
    HEARTBEAT_nest0_loop1(cols, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, i);
  }
}

// void HEARTBEAT_nest0_loop1(int cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW, int i) {
void HEARTBEAT_nest0_loop1(uint64_t cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW, uint64_t i) {
  // for (int j = 0; j < cols; j++) { 
  for (uint64_t j = 0; j < cols; j++) { 
  
    int k = i * cols + j;
    float Jc = J[k];

    // directional derivates
    dN[k] = J[iN[i] * cols + j] - Jc;
    dS[k] = J[iS[i] * cols + j] - Jc;
    dW[k] = J[i * cols + jW[j]] - Jc;
    dE[k] = J[i * cols + jE[j]] - Jc;
    
    float G2 = (dN[k]*dN[k] + dS[k]*dS[k] 
    + dW[k]*dW[k] + dE[k]*dE[k]) / (Jc*Jc);

    float L = (dN[k] + dS[k] + dW[k] + dE[k]) / Jc;

    float num  = (0.5*G2) - ((1.0/16.0)*(L*L)) ;
    float den  = 1 + (.25*L);
    float qsqr = num/(den*den);

    // diffusion coefficent (equ 33)
    den = (qsqr-q0sqr) / (q0sqr * (1+q0sqr)) ;
    c[k] = 1.0 / (1.0+den) ;
              
    // saturate diffusion coefficent
    if (c[k] < 0) {c[k] = 0;}
    else if (c[k] > 1) {c[k] = 1;}
  
  }
}

// void HEARTBEAT_nest1_loop0(int rows, int cols, float *J, float *dN, float *dS, float *dW, float *dE, float *c, int *iS, int *jE, float lambda) {
void HEARTBEAT_nest1_loop0(uint64_t rows, uint64_t cols, float *J, float *dN, float *dS, float *dW, float *dE, float *c, int *iS, int *jE, float lambda) {
  // for (int i = 0; i < rows; i++) {
  for (uint64_t i = 0; i < rows; i++) {
    HEARTBEAT_nest1_loop1(cols, J, dN, dS, dW, dE, c, iS, jE, lambda, i);
  }
}

// void HEARTBEAT_nest1_loop1(int cols, float *J, float *dN, float *dS, float *dW, float *dE, float *c, int *iS, int *jE, float lambda, int i) {
void HEARTBEAT_nest1_loop1(uint64_t cols, float *J, float *dN, float *dS, float *dW, float *dE, float *c, int *iS, int *jE, float lambda, uint64_t i) {
  // for (int j = 0; j < cols; j++) {        
  for (uint64_t j = 0; j < cols; j++) {        

    // current index
    int k = i * cols + j;
              
    // diffusion coefficent
    float cN = c[k];
    float cS = c[iS[i] * cols + j];
    float cW = c[k];
    float cE = c[i * cols + jE[j]];

    // divergence (equ 58)
    float D = cN * dN[k] + cS * dS[k] + cW * dW[k] + cE * dE[k];
              
    // image update (equ 61)
    J[k] = J[k] + 0.25*lambda*D;
  }
}

} // namespace srad
