#include "spmv.hpp"

using namespace spmv;

int main() {
  setup();

#if defined(USE_OPENMP)
  spmv_openmp(val, row_ptr, col_ind, x, y, nb_rows);
#endif

  finishup();

  return 0;
}