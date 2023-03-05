#pragma once

#include <cstdint>
#include <functional>

extern
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end);

namespace plus_reduce_array {

extern
double HEARTBEAT_nest0_loop0(double *a, uint64_t lo, uint64_t hi);

inline
double plus_reduce_array_hb_manual(double* a, uint64_t lo, uint64_t hi) {
  double r = HEARTBEAT_nest0_loop0(a, lo, hi);
  return r;
}

} // namespace plus_reduce_array
