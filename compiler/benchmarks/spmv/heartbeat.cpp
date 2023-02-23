#include "spmv.hpp"
#if defined(HEARTBEAT_VERSIONING)
#include "loop_handler.hpp"
#endif
#if defined(COLLECT_KERNEL_TIME)
#include <stdio.h>
#include <chrono>
#endif

void HEARTBEAT_loop0(double *val, uint64_t *row_ptr, uint64_t *col_ind, double *x, double *y, uint64_t n);
double HEARTBEAT_loop1(double *val, uint64_t startIter, uint64_t maxIter, uint64_t *col_ind, double *x, uint64_t i);

void HEARTBEAT_loop0(double *val, uint64_t *row_ptr, uint64_t *col_ind, double *x, double *y, uint64_t n) {
  for (uint64_t i = 0; i < n; i++) {
    uint64_t startIter = row_ptr[i];
    uint64_t maxIter = row_ptr[i + 1];
    y[i] = HEARTBEAT_loop1(val, startIter, maxIter, col_ind, x, i);
  }

  return;
}

double HEARTBEAT_loop1(double *val, uint64_t startIter, uint64_t maxIter, uint64_t *col_ind, double *x, uint64_t i) {
  double r = 0.0;
  for (uint64_t k = startIter; k < maxIter; k++) {
    r += val[k] * x[col_ind[k]];
  }

  return r;
}

bool run_heartbeat = true;

extern double *val;
extern uint64_t *row_ptr;
extern uint64_t *col_ind;
extern double *x;
extern double *y;
extern uint64_t nb_rows;

int main() {
  setup();

#if defined(COLLECT_KERNEL_TIME)
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif

  loop_dispatcher([&] {
    HEARTBEAT_loop0(val, row_ptr, col_ind, x, y, nb_rows);
  });

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif

#if defined(TEST_CORRECTNESS)
  test_correctness(y);
#endif

  finishup();

#if defined(HEARTBEAT_VERSIONING)
  // create dummy call here so loop_handler declarations
  // so that the function declaration will be emitted by the LLVM-IR
  loop_handler(nullptr, 0, nullptr, 0, 0, 0, 0);
#endif

  return 0;
}
