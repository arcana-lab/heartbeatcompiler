#include "spmv.hpp"

using namespace spmv;

int main() {
  setup();

  spmv_serial(val, row_ptr, col_ind, x, y, nb_rows);

  finishup();

  return 0;
}