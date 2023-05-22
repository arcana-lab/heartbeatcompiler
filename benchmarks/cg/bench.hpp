#pragma once

#include <cstdint>
#include <functional>

namespace cg {

#ifndef USE_FLOAT
using scalar = double;
#else
using scalar = float;
#endif

extern uint64_t n;
extern uint64_t *col_ind;
extern uint64_t *row_ptr;
extern scalar *x;
extern scalar *z;
extern scalar *val;
extern scalar *p;
extern scalar *q;
extern scalar *r;
extern scalar rnorm;

#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end);
#endif
void setup();
void finishup();

#if defined(USE_BASELINE)
scalar conj_grad_serial(
  uint64_t n,
  uint64_t* col_ind,
  uint64_t* row_ptr,
  scalar* x,
  scalar* z,
  scalar* val,
  scalar* p,
  scalar* q,
  scalar* r);
#elif defined(USE_OPENCILK)
scalar conj_grad_opencilk(
  uint64_t n,
  uint64_t* col_ind,
  uint64_t* row_ptr,
  scalar* x,
  scalar* z,
  scalar* val,
  scalar* p,
  scalar* q,
  scalar* r);
#elif defined(USE_OPENMP)
scalar conj_grad_openmp(
  uint64_t n,
  uint64_t* col_ind,
  uint64_t* row_ptr,
  scalar* x,
  scalar* z,
  scalar* val,
  scalar* p,
  scalar* q,
  scalar* r);
#endif

#if defined(TEST_CORRECTNESS)
void test_correctness();
#endif

} // namespace cg
