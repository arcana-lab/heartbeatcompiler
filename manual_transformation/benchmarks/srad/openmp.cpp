#include "srad.hpp"

using namespace srad;

int main() {

  setup();

#if defined(USE_OPENMP)
  srad_openmp(rows, cols, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
#endif

  finishup();

  return 0;
}