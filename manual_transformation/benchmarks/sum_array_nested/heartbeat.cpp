#include "sum_array_nested.hpp"
#include "stdio.h"
#if defined(HEARTBEAT_BRANCHES)
#include "heartbeat_branches.hpp"
#elif defined(HEARTBEAT_VERSIONING)
#include "heartbeat_versioning.hpp"
#else
#error "Need to specific the algorithm of heartbeat transformation: HEARTBEAT_{BRANCHES, VERSIONING}"
#endif
#if defined(COLLECT_KERNEL_TIME)
#include <stdio.h>
#include <chrono>
#endif

extern uint64_t n1;
extern uint64_t n2;
extern uint64_t result;
extern char **a;

void loop_dispatcher(
  void (*f)(char **, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t &),
  char **a,
  uint64_t low1,
  uint64_t high1,
  uint64_t low2,
  uint64_t high2,
  uint64_t &result
) {
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    (*f)(a, low1, high1, low2, high2, result);
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
  loop_dispatcher(&sum_array_nested_heartbeat_branches, a, 0, n1, 0, n2, result);
#elif defined(HEARTBEAT_VERSIONING)
  loop_dispatcher(&sum_array_nested_heartbeat_versioning, a, 0, n1, 0, n2, result);
#else
  #error "Need to specific the algorithm of heartbeat transformation: HEARTBEAT_{BRANCHES, VERSIONING}"
#endif

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif
  finishup();

  printf("result=%lu\n", result);
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  collect_heartbeat_polling_time_print();
#endif
#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  collect_heartbeat_promotion_time_print();
#endif
  return 0;
}
