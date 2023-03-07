#pragma once

#include <functional>

namespace srad {

extern int rows, cols;
extern int size_I, size_R;
extern float *I, *J, q0sqr;
extern float *dN, *dS, *dW, *dE;
extern float *c;
extern int *iN, *iS, *jE, *jW;
extern float lambda;

#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end);
#endif
void setup();
void finishup();

#if defined(USE_BASELINE)
  void srad_serial(int rows, int cols, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda);
#elif defined(USE_OPENCILK)
  void srad_opencilk(int rows, int cols, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda);
#elif defined(USE_OPENMP)
  void srad_openmp(int rows, int cols, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda);
#endif

#if TEST_CORRECTNESS
void test_correctness();
#endif

} // namespace srad
