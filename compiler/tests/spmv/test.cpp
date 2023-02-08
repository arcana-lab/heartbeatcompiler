#include "spmv.hpp"
#if defined(USE_HEARTBEAT)
#include "loop_handler.hpp"
// #include "loop_handler_temp.hpp"
#endif
#include <cstdint>
#include <chrono>

void HEARTBEAT_loop0(double *val, uint64_t *row_ptr, uint64_t *col_ind, double *x, double *y, uint64_t n);
double HEARTBEAT_loop1(double *val, uint64_t startIter, uint64_t maxIter, uint64_t *col_ind, double *x, uint64_t i);

void HEARTBEAT_loop0(double *val, uint64_t *row_ptr, uint64_t *col_ind, double *x, double *y, uint64_t n) {
  // constant liveIns:
  // row_ptr  at index 1
  // y        at index 4
  for (uint64_t i = 0; i < n; i++) {
    uint64_t startIter = row_ptr[i];
    uint64_t maxIter = row_ptr[i + 1];
    y[i] = HEARTBEAT_loop1(val, startIter, maxIter, col_ind, x, i);
  }

  return;
}

double HEARTBEAT_loop1(double *val, uint64_t startIter, uint64_t maxIter, uint64_t *col_ind, double *x, uint64_t i) {
  // constant liveIns:
  // val      at original index 0
  // col_ind  at original index 2
  // x        at original index 3
  double r = 0.0;
  for (uint64_t k = startIter; k < maxIter; k++) {
    r += val[k] * x[col_ind[k]];
  }

  return r;
}

bool runHeartbeat = true;

int main(int argc, char *argv[]) {

  setup();

  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();

#if defined(USE_BASELINE)
  HEARTBEAT_loop0(val, row_ptr, col_ind, x, y, nb_rows);
#elif defined(USE_HEARTBEAT)
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    HEARTBEAT_loop0(val, row_ptr, col_ind, x, y, nb_rows);
  });
#endif

  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());

  finishup();

  test_correctness(y);

  return 0;
}
