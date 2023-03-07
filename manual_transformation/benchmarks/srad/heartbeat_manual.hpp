#pragma once

#include <functional>

extern
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end);

namespace srad {

void HEARTBEAT_nest0_loop0(int rows, int cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW);
void HEARTBEAT_nest1_loop0(int rows, int cols, float *J, float *dN, float *dS, float *dW, float *dE, float *c, int *iS, int *jE, float lambda);

inline
void srad_hb_manual(int rows, int cols, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda) {
  HEARTBEAT_nest0_loop0(rows, cols, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW);
  HEARTBEAT_nest1_loop0(rows, cols, J, dN, dS, dW, dE, c, iS, jE, lambda);
}

} // namespace srad
