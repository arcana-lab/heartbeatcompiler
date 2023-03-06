#pragma once

#include <cstdint>
#include <functional>

namespace plus_reduce_array {

double HEARTBEAT_nest0_loop0(double *a, uint64_t lo, uint64_t hi) {
  double r = 0.0;
  for (uint64_t i = lo; i != hi; i++) {
    r += a[i];
  }
  return r;
}

double plus_reduce_array_hb_compiler(double* a, uint64_t lo, uint64_t hi) {
  double r = HEARTBEAT_nest0_loop0(a, lo, hi);
  return r;
}

} // namespace plus_reduce_array
