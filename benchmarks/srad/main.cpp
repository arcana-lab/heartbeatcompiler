#include "srad.hpp"
#if defined(COLLECT_KERNEL_TIME)
#include <stdio.h>
#include <chrono>
#endif

extern int rows, cols;
extern float *I, *J, q0sqr;
extern float *dN, *dS, *dW, *dE;
extern float *c;
extern int *iN, *iS, *jE, *jW;
extern float lambda;

int main() {
  setup();

#if defined(COLLECT_KERNEL_TIME)
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif

#if defined(USE_BASELINE)
  srad_serial(rows, cols, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
#elif defined(USE_OPENCILK)
  srad_opencilk(rows, cols, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
#elif defined(USE_OPENMP)
  srad_openmp(rows, cols, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
#endif

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif

#if TEST_CORRECTNESS
  test_correctness();
#endif

  finishup();

  return 0;
}
