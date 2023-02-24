#pragma once

#include <cstdint>

void setup();
void finishup();

#if defined(USE_BASELINE)
  double sum_array_serial(double *a, uint64_t lo, uint64_t hi);
#elif defined(USE_OPENCILK)
  double sum_array_opencilk(double *a, uint64_t lo, uint64_t hi);
#elif defined(USE_OPENMP)
  double sum_array_openmp(double* a, uint64_t lo, uint64_t hi);
#endif
