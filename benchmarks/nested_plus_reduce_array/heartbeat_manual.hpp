#pragma once

#include "loop_handler.hpp"
#include <cstdint>

namespace nested_plus_reduce_array {

double HEARTBEAT_nest0_loop0(double **a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2);

inline
double nested_plus_reduce_array_hb_manual(double **a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2) {
  return HEARTBEAT_nest0_loop0(a, lo1, hi1, lo2, hi2);
}

} // namespace nested_plus_reduce_array