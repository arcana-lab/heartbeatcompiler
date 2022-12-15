#include "spmv.hpp"
#if defined(HEARTBEAT_BRANCHES)
#include "heartbeat_branches.hpp"
#elif defined(HEARTBEAT_VERSIONING)
#include "heartbeat_versioning.hpp"
#else
#error "Need to specific the version of heartbeat, e.g., HEARTBEAT_BRANCHES, HEARTBEAT_VERSIONING"
#endif
#if defined(COLLECT_KERNEL_TIME)
#include <stdio.h>
#include <chrono>
#endif

using namespace spmv;

void loop_dispatcher(
  void (*f)(double *, uint64_t *, uint64_t *, double *, double *, uint64_t),
  double *val,
  uint64_t *row_ptr,
  uint64_t *col_ind,
  double *x,
  double *y,
  uint64_t n
) {
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    (*f)(val, row_ptr, col_ind, x, y, n);
  });
}

int main() {
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  collect_heartbeat_polling_time_init();
#endif
#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  collect_heartbeat_promotion_time_init();
#endif
  setup();

#if defined(COLLECT_KERNEL_TIME)
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif

#if defined(HEARTBEAT_BRANCHES)
  loop_dispatcher(&spmv_heartbeat_branches, val, row_ptr, col_ind, x, y, nb_rows);
#elif defined(HEARTBEAT_VERSIONING)
  loop_dispatcher(&spmv_heartbeat_versioning, val, row_ptr, col_ind, x, y, nb_rows);
#else
  #error "Need to specific the version of heartbeat, e.g., HEARTBEAT_BRANCHES, HEARTBEAT_VERSIONING"
#endif

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif

#if defined(TEST_CORRECTNESS)
  test_correctness(y);
#endif

  finishup();

#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  collect_heartbeat_polling_time_print();
#endif
#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  collect_heartbeat_promotion_time_print();
#endif
  return 0;
}