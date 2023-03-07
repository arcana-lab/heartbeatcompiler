#pragma once

#include <cstdint>
#include <functional>

extern
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end);

namespace spmv {

void HEARTBEAT_nest0_loop0(double *val, uint64_t *row_ptr, uint64_t *col_ind, double* x, double* y, uint64_t n);

inline
void spmv_hb_manual(double *val, uint64_t *row_ptr, uint64_t *col_ind, double* x, double* y, uint64_t n) {
  HEARTBEAT_nest0_loop0(val, row_ptr, col_ind, x, y, n);
}

} // namespace spmv
