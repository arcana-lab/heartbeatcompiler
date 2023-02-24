#include "bench.hpp"
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

extern int vertices;
extern int *dist;

void loop_dispatcher(
  void (*f)(int *, int),
  int *dist,
  int vertices
) {
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    (*f)(dist, vertices);
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
  loop_dispatcher(&floyd_warshall_heartbeat_branches, dist, vertices);
#elif defined(HEARTBEAT_VERSIONING)
  loop_dispatcher(&floyd_warshall_heartbeat_versioning, dist, vertices);
#else
  #error "Need to specific the version of heartbeat, e.g., HEARTBEAT_BRANCHES, HEARTBEAT_VERSIONING"
#endif

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif

#if defined(TEST_CORRECTNESS)
  test_correctness(dist);
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
