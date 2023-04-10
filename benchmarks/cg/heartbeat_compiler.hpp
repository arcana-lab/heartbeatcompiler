#pragma once

#include "loop_handler.hpp"
#include <cstdint>

namespace cg {

void HEARTBEAT_nest0_loop0(double *val, uint64_t *row_ptr, uint64_t *col_ind, double* x, double* y, uint64_t n);
double HEARTBEAT_nest0_loop1(double *val, uint64_t startIter, uint64_t maxIter, uint64_t *col_ind, double* x, double* y, int i);

// Outlined loops
void HEARTBEAT_nest0_loop0(double *val, uint64_t *row_ptr, uint64_t *col_ind, double* x, double* y, uint64_t n) {
  for (uint64_t i = 0; i < n; i++) { // row loop
    double r = 0.0;
    uint64_t startIter = row_ptr[i];
    uint64_t maxIter = row_ptr[i + 1];
    r += HEARTBEAT_nest0_loop1(val, startIter, maxIter, col_ind, x, y, i);
    y[i] = r;
  }
}

// double HEARTBEAT_nest0_loop1(double *val, uint64_t *row_ptr, uint64_t *col_ind, double* x, double* y, int i) {
double HEARTBEAT_nest0_loop1(double *val, uint64_t startIter, uint64_t maxIter, uint64_t *col_ind, double* x, double* y, int i) {
  double r = 0.0;
  // for (uint64_t k = row_ptr[i]; k < row_ptr[i + 1]; k++) { // col loop
  for (uint64_t k = startIter; k < maxIter; k++) { // col loop
    r += val[k] * x[col_ind[k]];
  }
  return r;
}

void HEARTBEAT_nest0_loop2(
  uint64_t lo,
  uint64_t hi,
  double* q,
  double* z,
  double* r,
  double* p) {
  for (uint64_t j = lo; j < hi; j++) {
    q[j] = 0.0;
    z[j] = 0.0;
    r[j] = x[j];
    p[j] = r[j];
  }
}

double HEARTBEAT_nest0_loop3(
  uint64_t lo,
  uint64_t hi,
  double* r) {
  double rho = 0.0;
  for (uint64_t j = lo; j < hi; j++) {
    rho = rho + r[j]*r[j];
  }
  return rho;
}

double HEARTBEAT_nest0_loop4(double *p, double* q, uint64_t startIter, uint64_t maxIter) {
  double r = 0.0;
  for (uint64_t k = startIter; k < maxIter; k++) {
    r += p[k] * q[k];
  }
  return r;
}

double HEARTBEAT_nest0_loop5(uint64_t lo, uint64_t hi,
			     double* z, double* r, double* p, double* q, double alpha) {
  double rho = 0.0;
  for (j = lo; j <= hi; j++) {
    z[j] = z[j] + alpha*p[j];
    r[j] = r[j] - alpha*q[j];
    rho = rho + r[j]*r[j];
 }
  return rho;
}

void HEARTBEAT_nest0_loop6(uint64_t lo, uint64_t hi, double* p, double* r, double beta) {
  for (j = lo; j < hi; j++) {
    p[j] = r[j] + beta*p[j];
  }
}

double HEARTBEAT_nest0_loop6(uint64_t lo, uint64_t hi, double* x, double* r) {
  double sum = 0.0;
  for (j = lo; j < hi; j++) {
    double d = x[j] - r[j];
    sum = sum + d*d;
  }
  return sum;
}
  
void conj_grad_compiler(
  uint64_t* col_ind,
  uint64_t* row_ptr,
  double* x,
  double* z,
  double* a,
  double* p,
  double* q,
  double* r,
  double *rnorm ) {
  uint64_t callcount = 0;
  double d, rho, rho0, alpha, beta;
  uint64_t i, j, k;
  uint64_t cgit, cgitmax = 25;
  rho = 0.0;
  HEARTBEAT_nest0_loop2(0, naa, q, z, r, p);
  rho = HEARTBEAT_nest0_loop3(0, lastcol-firstcol);
  for (cgit = 1; cgit <= cgitmax; cgit++) {
    rho0 = rho;
    d = 0.0;
    rho = 0.0;
    HEARTBEAT_nest0_loop0(a, row_ptr, col_ind, p, q, lastrow-firstrow);
    d += HEARTBEAT_nest0_loop4(p, q, 0, lastcol-firstcol);
    alpha = rho0 / d;
    rho += HEARTBEAT_nest0_loop5(0, lastcol-firstcol, z, r, p, q, alpha);
    beta = rho / rho0;
    HEARTBEAT_nest0_loop6(0, lastcol-firstcol, p, r, beta);
    callcount++;
  }
  HEARTBEAT_nest0_loop0(a, row_ptr, col_ind, z, r, lastrow-firstrow);
  double sum = HEARTBEAT_nest0_loop6(0, lastcol-firstcol, x, r);
  *rnorm = sqrt(sum);
}
  
} // namespace cg
