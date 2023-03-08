#pragma once

#include "loop_handler.hpp"
#include <cstdint>

namespace plus_reduce_array {

double HEARTBEAT_nest0_loop0(double *a, uint64_t lo, uint64_t hi);

inline
double plus_reduce_array_hb_manual(double* a, uint64_t lo, uint64_t hi) {
  double r = HEARTBEAT_nest0_loop0(a, lo, hi);
  return r;
}

} // namespace plus_reduce_array
