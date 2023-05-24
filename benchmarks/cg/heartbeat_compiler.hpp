#pragma once

#include "loop_handler.hpp"
#include <cstdint>
#include <cmath>

namespace cg {

void HEARTBEAT_nest0_loop0(
  uint64_t n,
  scalar* x,
  scalar* z,
  scalar* p,
  scalar* q,
  scalar* r
);

scalar HEARTBEAT_nest1_loop0(
  uint64_t n,
  scalar *q,
  scalar *r
);

void HEARTBEAT_nest2_loop0(
  scalar* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  scalar* x,
  scalar* y,
  uint64_t n
);

scalar HEARTBEAT_nest2_loop1(
  scalar* val,
  uint64_t* col_ind,
  scalar* x,
  uint64_t startIter,
  uint64_t maxIter
);

scalar HEARTBEAT_nest3_loop0(
  uint64_t n,
  scalar* z,
  scalar* p,
  scalar* q,
  scalar* r,
  scalar alpha
);

void HEARTBEAT_nest4_loop0(
  uint64_t n,
  scalar *p,
  scalar *r,
  scalar beta);

scalar HEARTBEAT_nest5_loop0(
  uint64_t n,
  scalar *x,
  scalar *r
);

scalar dotp_hb_compiler(uint64_t n, scalar* r, scalar* q) {
  scalar sum = 0.0;
  sum += HEARTBEAT_nest1_loop0(n, q, r);
  return sum;
}

scalar norm_hb_compiler(uint64_t n, scalar* r) {
  return dotp_hb_compiler(n, r, r);
}

void spmv_hb_compiler(
  scalar* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  scalar* x,
  scalar* y,
  uint64_t n) {
  HEARTBEAT_nest2_loop0(val, row_ptr, col_ind, x, y, n);
}

scalar conj_grad_hb_compiler(
    uint64_t n,
    uint64_t* col_ind,
    uint64_t* row_ptr,
    scalar* x,
    scalar* z,
    scalar* a,
    scalar* p,
    scalar* q,
    scalar* r) {
  int cgitmax = 25;

/*--------------------------------------------------------------------
c  Initialize the CG algorithm:
c-------------------------------------------------------------------*/
  HEARTBEAT_nest0_loop0(n, x, z, p, q, r);

/*--------------------------------------------------------------------
c  rho = r.r
c  Now, obtain the norm of r: First, sum squares of r elements locally...
c-------------------------------------------------------------------*/
  scalar rho = norm_hb_compiler(n, r);

/*--------------------------------------------------------------------
c---->
c  The conj grad iteration loop
c---->
c-------------------------------------------------------------------*/
  for (uint64_t cgit = 0; cgit < cgitmax; cgit++) {
    scalar rho0 = rho;
    rho = 0.0;

/*--------------------------------------------------------------------
c  q = A.p
c-------------------------------------------------------------------*/
    spmv_hb_compiler(a, row_ptr, col_ind, p, q, n);

/*--------------------------------------------------------------------
c  Obtain alpha = rho / (p.q)
c-------------------------------------------------------------------*/
    scalar alpha = rho0 / dotp_hb_compiler(n, p, q);

/*---------------------------------------------------------------------
c  Obtain z = z + alpha*p
c  and    r = r - alpha*q
c---------------------------------------------------------------------*/
    rho += HEARTBEAT_nest3_loop0(n, z, p, q, r, alpha);
    scalar beta = rho / rho0;

/*--------------------------------------------------------------------
c  p = r + beta*p
c-------------------------------------------------------------------*/
    HEARTBEAT_nest4_loop0(n, p, r, beta);
  }

  spmv_hb_compiler(a, row_ptr, col_ind, z, r, n);

/*--------------------------------------------------------------------
c  At this point, r contains A.z
c-------------------------------------------------------------------*/
  scalar sum = 0.0;
  sum += HEARTBEAT_nest5_loop0(n, x, r);
  return sqrt(sum);
}

void HEARTBEAT_nest0_loop0(
  uint64_t n,
  scalar* x,
  scalar* z,
  scalar* p,
  scalar* q,
  scalar* r
) {
  // parallel for
  for (uint64_t j = 0; j < n; j++) {
    q[j] = 0.0;
    z[j] = 0.0;
    r[j] = x[j];
    p[j] = r[j];
  }
}

scalar HEARTBEAT_nest1_loop0(
  uint64_t n,
  scalar *q,
  scalar *r
) {
  scalar sum = 0.0;
  // parallel for reduction(+:sum)
  for (uint64_t j = 0; j < n; j++) {
    sum += r[j] * q[j];
  }
  return sum;
}

void HEARTBEAT_nest2_loop0(
  scalar* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  scalar* x,
  scalar* y,
  uint64_t n
) {
  // parallel for
  for (uint64_t i = 0; i < n; i++) { // row loop
    scalar r = 0.0;
    uint64_t startIter = row_ptr[i];
    uint64_t maxIter = row_ptr[i + 1];
    r += HEARTBEAT_nest2_loop1(val, col_ind, x, startIter, maxIter);
    y[i] = r;
  }
}

scalar HEARTBEAT_nest2_loop1(
  scalar* val,
  uint64_t* col_ind,
  scalar* x,
  uint64_t startIter,
  uint64_t maxIter
) {
  scalar r = 0.0;
  // parallel for reduction(+:r)
  for (uint64_t k = startIter; k < maxIter; k++) { // col loop
    r += val[k] * x[col_ind[k]];
  }
  return r;
}

scalar HEARTBEAT_nest3_loop0(
  uint64_t n,
  scalar* z,
  scalar* p,
  scalar* q,
  scalar* r,
  scalar alpha
) {
  scalar rho = 0.0;
  // parallel for reduction(:+rho)
  for (uint64_t j = 0; j < n; j++) {
    z[j] = z[j] + alpha * p[j];
    r[j] = r[j] - alpha * q[j];
    rho += r[j] * r[j];
  }
  return rho;
}

void HEARTBEAT_nest4_loop0(
  uint64_t n,
  scalar *p,
  scalar *r,
  scalar beta) {
  // parallel for
  for (uint64_t j = 0; j < n; j++) {
    p[j] = r[j] + beta * p[j];
  }
}

scalar HEARTBEAT_nest5_loop0(
  uint64_t n,
  scalar *x,
  scalar *r
) {
  scalar sum = 0.0;
  // parallel for reduction(+:sum)
  for (uint64_t j = 0; j < n; j++) {
    scalar d = x[j] - r[j];
    sum += d * d;
  }
  return sum;
}

} // namespace cg
