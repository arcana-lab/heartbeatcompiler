#include "spmv.hpp"

using namespace spmv;

int main() {

  setup();

#if defined(USE_OPENCILK)
  spmv_opencilk(val, row_ptr, col_ind, _spmv_x, _spmv_y, nb_rows);
#endif

  finishup();

  return 0;
}