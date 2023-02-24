#pragma once

void setup();
void finishup();

#if defined(USE_BASELINE)
  void srad_serial(int rows, int cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW, float lambda);
#elif defined(USE_OPENCILK)
  void srad_opencilk(int rows, int cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW, float lambda);
#elif defined(USE_OPENMP)
  void srad_openmp(int rows, int cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW, float lambda);
#endif

#if TEST_CORRECTNESS
  void test_correctness();
#endif
