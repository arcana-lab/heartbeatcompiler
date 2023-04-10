#pragma once

#include <cstdint>
#include <functional>

namespace cg {

#ifndef USE_FLOAT
using scalar = double;
#else
using scalar = float;
#endif

#include <taskparts/benchmark.hpp>

uint64_t n;
uint64_t nb_vals;
scalar* val;
uint64_t* row_ptr;
uint64_t* col_ind;
scalar* x;
scalar* y;
scalar *p;
scalar *q;
scalar* r;
  
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
  scalar* a,
  scalar* p,
  scalar* q,
  scalar* r,
  uint64_t cgitmax);
#elif defined(USE_OPENCILK)
scalar conj_grad_cilk(
  uint64_t n,
  uint64_t* col_ind,
  uint64_t* row_ptr,
  scalar* x,
  scalar* z,
  scalar* a,
  scalar* p,
  scalar* q,
  scalar* r,
  uint64_t cgitmax);
#elif defined(USE_OPENMP)
scalar conj_grad_openmp(
  uint64_t n,
  uint64_t* col_ind,
  uint64_t* row_ptr,
  scalar* x,
  scalar* z,
  scalar* a,
  scalar* p,
  scalar* q,
  scalar* r,
  uint64_t cgitmax);
#endif

#if defined(TEST_CORRECTNESS)
void test_correctness();
#endif

} // namespace cg
