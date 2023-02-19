#pragma once

#include <cstdint>

void setup();
void finishup();

#if defined(USE_BASELINE)
  uint64_t sum_array_nested_serial(char **a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2);
#elif defined(USE_OPENCILK)
  uint64_t sum_array_nested_opencilk(char **a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2);
#elif defined(USE_OPENMP)
  uint64_t sum_array_nested_openmp(char **a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2);
#endif
