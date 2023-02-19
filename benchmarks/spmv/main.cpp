#include "spmv.hpp"
#if defined(COLLECT_KERNEL_TIME)
#include <stdio.h>
#include <chrono>
#endif

extern double* val;
extern uint64_t* row_ptr;
extern uint64_t* col_ind;
extern double* x;
extern double* y;
extern uint64_t nb_rows;

int main() {
  setup();

#if defined(COLLECT_KERNEL_TIME)
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif

#if defined(USE_BASELINE)
  spmv_serial(val, row_ptr, col_ind, x, y, nb_rows);
#elif defined(USE_OPENCILK)
  spmv_opencilk(val, row_ptr, col_ind, x, y, nb_rows);
#elif defined(USE_OPENMP)
  spmv_openmp(val, row_ptr, col_ind, x, y, nb_rows);
#endif

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif

#if defined(TEST_CORRECTNESS)
  test_correctness(y);
#endif

  finishup();

  return 0;
}
