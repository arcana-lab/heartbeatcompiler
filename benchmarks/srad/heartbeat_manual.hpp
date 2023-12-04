#pragma once

#include <functional>

#include "loop_handler.hpp"

namespace srad {

void srad_hb_manual(int rows, int cols, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda);

} // namespace srad
