#include "spmv.hpp"

using namespace spmv;

int main() {

  setup();

  spmv_serial(val, row_ptr, col_ind, _spmv_x, _spmv_y, nb_rows);

  finishup();

  return 0;
}