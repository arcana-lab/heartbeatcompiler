#pragma once

#include <cstdint>

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
  void test_correctness(double* y);
#endif
