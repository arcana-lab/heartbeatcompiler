#pragma once

#include "loop_handler.hpp"
#include <cstdint>

#if defined(ACC_SPMV_STATS)
typedef struct {
  uint64_t startIter;
  uint64_t chunksize;
} ass_t;
extern ass_t *ass_stats;
extern uint64_t ass_begin;
#endif

namespace spmv {

#if !defined(ACC_SPMV_STATS) && !defined(SPMV_DETECTION_RATE_ANALYSIS)
// default parallel annotation for spmv at both outer and inner levels

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

#else
// another parallel annotation for spmv for outer loop only,
// therefore inner loop shows heterogeneous latency
// the purpose of this annotation is to demonstrate how ACC changes chunk size
// in reverse relationship to the number of non-zero elements processed

#include <stdio.h>
#include <cassert>

void HEARTBEAT_nest0_loop0(double *val, uint64_t *row_ptr, uint64_t *col_ind, double* x, double* y, uint64_t n);

void spmv_hb_compiler(double *val, uint64_t *row_ptr, uint64_t *col_ind, double* x, double* y, uint64_t n) {
  HEARTBEAT_nest0_loop0(val, row_ptr, col_ind, x, y, n);
#if defined(ACC_SPMV_STATS)
  // print the nonzeros and chunksize per row
  uint64_t ass_index = 1;
  for (uint64_t i = 0; i < n; i++) {
    printf("%lu\t%lu\t", i, row_ptr[i+1] - row_ptr[i]);
    if (ass_index == ass_begin) {
      printf("%lu\n", ass_stats[ass_index-1].chunksize);
    } else if (i < ass_stats[ass_index].startIter) {
      printf("%lu\n", ass_stats[ass_index-1].chunksize);
    } else {
      assert (i >= ass_stats[ass_index].startIter);
      printf("%lu\n", ass_stats[ass_index].chunksize);
      ass_index++;
    }
  }
#endif
}

// Outlined loops
void HEARTBEAT_nest0_loop0(double *val, uint64_t *row_ptr, uint64_t *col_ind, double* x, double* y, uint64_t n) {
  for (uint64_t i = 0; i < n; i++) { // row loop
    double r = 0.0;
    for (uint64_t k = row_ptr[i]; k < row_ptr[i + 1]; k++) { // col loop
      r += val[k] * x[col_ind[k]];
    }
    y[i] = r;
  }
}

#endif

} // nameapce spmv
