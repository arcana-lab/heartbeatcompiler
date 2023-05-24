#pragma once

#include "loop_handler.hpp"
#include <cstdint>

namespace spmv {

void HEARTBEAT_nest0_loop0(double *val, uint64_t *row_ptr, uint64_t *col_ind, double* x, double* y, uint64_t n);
double HEARTBEAT_nest0_loop1(double *val, uint64_t startIter, uint64_t maxIter, uint64_t *col_ind, double* x, double* y);

void spmv_hb_compiler(double *val, uint64_t *row_ptr, uint64_t *col_ind, double* x, double* y, uint64_t n) {
  HEARTBEAT_nest0_loop0(val, row_ptr, col_ind, x, y, n);
}

// Outlined loops
void HEARTBEAT_nest0_loop0(double *val, uint64_t *row_ptr, uint64_t *col_ind, double* x, double* y, uint64_t n) {
  for (uint64_t i = 0; i < n; i++) { // row loop
    double r = 0.0;
    uint64_t startIter = row_ptr[i];
    uint64_t maxIter = row_ptr[i + 1];
    r += HEARTBEAT_nest0_loop1(val, startIter, maxIter, col_ind, x, y);
    y[i] = r;
  }
}

// double HEARTBEAT_nest0_loop1(double *val, uint64_t *row_ptr, uint64_t *col_ind, double* x, double* y, int i) {
double HEARTBEAT_nest0_loop1(double *val, uint64_t startIter, uint64_t maxIter, uint64_t *col_ind, double* x, double* y) {
  double r = 0.0;
  // for (uint64_t k = row_ptr[i]; k < row_ptr[i + 1]; k++) { // col loop
  for (uint64_t k = startIter; k < maxIter; k++) { // col loop
    r += val[k] * x[col_ind[k]];
  }
  return r;
}

} // nameapce spmv
