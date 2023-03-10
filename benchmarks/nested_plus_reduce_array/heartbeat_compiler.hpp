#pragma once

#include "loop_handler.hpp"
#include <cstdint>

namespace nested_plus_reduce_array {

double HEARTBEAT_nest0_loop0(double **a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2);
double HEARTBEAT_nest0_loop1(double **a, uint64_t lo2, uint64_t hi2, uint64_t i);

double nested_plus_reduce_array_hb_manual(double **a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2) {
  double r = HEARTBEAT_nest0_loop0(a, lo1, hi1, lo2, hi2);
  return r;
}

// Outlined loops
double HEARTBEAT_nest0_loop0(double **a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2) {
  double r = 0.0;
  for (uint64_t i = lo1; i != hi1; i++) {
    r += HEARTBEAT_nest0_loop1(a, lo2, hi2, i);
  }
  return r;
}

double HEARTBEAT_nest0_loop1(double **a, uint64_t lo2, uint64_t hi2, uint64_t i) {
  double r = 0.0;
  for (uint64_t j = lo2; j != hi2; j++) {
    r += a[i][j];
  }
  return r;
}

} // namespace nested_plus_reduce_array
