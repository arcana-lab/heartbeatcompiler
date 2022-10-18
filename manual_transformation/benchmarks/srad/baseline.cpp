#include "srad.hpp"

using namespace srad;

int main() {

  setup();

  srad_serial(rows, cols, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);

  finishup();

  return 0;
}