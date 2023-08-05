#pragma once

#include "loop_handler.hpp"
#include <cstdint>

namespace spmv {

void spmv_hb_manual(double *val, uint64_t *row_ptr, uint64_t *col_ind, double* x, double* y, uint64_t n);

} // namespace spmv
