#pragma once

#include "loop_handler.hpp"
#include <cstdint>

namespace plus_reduce_array {

double HEARTBEAT_loop0(double *a, uint64_t lo, uint64_t hi);

double plus_reduce_array_hb_compiler(double* a, uint64_t lo, uint64_t hi) {
  double r = HEARTBEAT_loop0(a, lo, hi);
  return r;
}

// Outlined loops
double HEARTBEAT_loop0(double *a, uint64_t lo, uint64_t hi) {
  double r = 0.0;
  for (uint64_t i = lo; i != hi; i++) {
    r += a[i];
  }
  return r;
}

} // namespace plus_reduce_array
