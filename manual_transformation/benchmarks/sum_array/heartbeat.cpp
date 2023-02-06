#include "sum_array.hpp"
#include "stdio.h"
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

void loop_dispatcher(
  void (*f)(double *, uint64_t, uint64_t, double &),
  double *a,
  uint64_t low,
  uint64_t high,
  double &result
) {
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    (*f)(a, low, high, result);
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
  loop_dispatcher(&sum_array_heartbeat_branches, a, 0, n, result);
#elif defined(HEARTBEAT_VERSIONING)
  loop_dispatcher(&sum_array_heartbeat_versioning, a, 0, n, result);
#else
  #error "Need to specific the version of heartbeat, e.g., HEARTBEAT_BRANCHES, HEARTBEAT_VERSIONING"
#endif

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif
  finishup();

  printf("result=%f\n", result);
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  collect_heartbeat_polling_time_print();
#endif
#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  collect_heartbeat_promotion_time_print();
#endif
  return 0;
}
