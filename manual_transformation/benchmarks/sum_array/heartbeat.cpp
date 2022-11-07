#include "sum_array.hpp"
#include "stdio.h"
#if defined(HEARTBEAT_BRANCHES)
#include "heartbeat_branches.hpp"
#elif defined(HEARTBEAT_VERSIONING) || defined(HEARTBEAT_VERSIONING_OPTIMIZED)
#include "heartbeat_versioning.hpp"
#else
#error "Need to specific the version of heartbeat, e.g., HEARTBEAT_BRANCHES, HEARTBEAT_VERSIONING
#endif
#if defined(COLLECT_KERNEL_TIME)
#include <stdio.h>
#include <chrono>
#endif

using namespace sum_array;

int main() {
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
#elif defined(HEARTBEAT_VERSIONING_OPTIMIZED)
  loop_dispatcher(&sum_array_heartbeat_versioning_optimized, a, 0, n, result);
#else
  #error "Need to specific the version of heartbeat, e.g., HEARTBEAT_BRANCHES, HEARTBEAT_VERSIONING
#endif

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif
  finishup();

  printf("result=%lu\n", result);
  return 0;
}
