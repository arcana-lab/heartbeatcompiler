#pragma once

#include <cstdint>
#include <functional>

namespace spmv {

extern double* val;
extern uint64_t* row_ptr;
extern uint64_t* col_ind;
extern double* x;
extern double* y;
extern uint64_t nb_rows;

#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end);
#endif
void setup();
void finishup();

#if defined(USE_BASELINE)
void spmv_serial(
  double *val,
  uint64_t *row_ptr,
  uint64_t *col_ind,
  double* x,
  double* y,
  uint64_t n
);
#elif defined(USE_OPENCILK)
void spmv_opencilk(
  double *val,
  uint64_t *row_ptr,
  uint64_t *col_ind,
  double* x,
  double* y,
  uint64_t n
);
#elif defined(USE_CILKPLUS)
void spmv_cilkplus(
  double *val,
  uint64_t *row_ptr,
  uint64_t *col_ind,
  double* x,
  double* y,
  uint64_t n
);
#elif defined(USE_OPENMP)
void spmv_openmp(
  double *val,
  uint64_t *row_ptr,
  uint64_t *col_ind,
  double* x,
  double* y,
  uint64_t n
);
#endif

#if defined(TEST_CORRECTNESS)
void test_correctness();
#endif

} // namespace spmv
