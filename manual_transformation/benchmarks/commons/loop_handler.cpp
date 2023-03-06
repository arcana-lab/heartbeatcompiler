#include "loop_handler.hpp"
#include <cstdint>
#include <functional>
#include <taskparts/benchmark.hpp>
#if defined(ENABLE_ROLLFORWARD)
#include <rollforward.h>
#endif

void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end) {
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    bench_body();
  }, [&] (auto sched) {
    bench_start();
  }, [&] (auto sched) {
    bench_end();
  });
}

#ifndef SMALLEST_GRANULARITY
  #error "Macro SMALLEST_GRANULARITY undefined"
#endif

#define CACHELINE 8
#define LIVE_IN_ENV 0
#define LIVE_OUT_ENV 1

int loop_handler_level1(uint64_t *cxt,
                        int (*sliceTask)(uint64_t *, uint64_t, uint64_t, uint64_t),
                        uint64_t startIter, uint64_t maxIter) {
#if defined(DISABLE_HEARTBEAT_PROMOTION)
  return 0;
#endif

#if !defined(ENABLE_ROLLFORWARD)
  /*
   * Determine whether to promote since last promotion
   */
  auto &p = taskparts::prev.mine();
  auto n = taskparts::cycles::now();
  if ((p + taskparts::kappa_cycles) > n) {
    return 0;
  }
  p = n;
#endif

  /*
   * Determine if there's more work for splitting
   */
  if ((maxIter - startIter) <= SMALLEST_GRANULARITY) {
    return 0;
  }

  /*
   * Calculate the splitting point of the rest of iterations
   */
  uint64_t mid = (startIter + 1 + maxIter) / 2;

  /*
   * Allocate cxts for both tasks
   */
  uint64_t cxtFirst[1 * CACHELINE];
  uint64_t cxtSecond[1 * CACHELINE];

  /*
   * Construct cxts for both tasks
   */
  cxtFirst[LIVE_IN_ENV]   = cxt[LIVE_IN_ENV];
  cxtSecond[LIVE_IN_ENV]  = cxt[LIVE_IN_ENV];
  cxtFirst[LIVE_OUT_ENV]  = cxt[LIVE_OUT_ENV];
  cxtSecond[LIVE_OUT_ENV] = cxt[LIVE_OUT_ENV];

  /*
   * Invoke splitting tasks
   */
  taskparts::tpalrts_promote_via_nativefj([&] {
    (*sliceTask)(cxtFirst, 0, startIter + 1, mid);
  }, [&] {
    (*sliceTask)(cxtSecond, 1, mid, maxIter);
  }, [] { }, taskparts::bench_scheduler());

  return 1;
}

#if defined(ENABLE_ROLLFORWARD)

void rollforward_handler_annotation __rf_handle_level1_wrapper(int &rc,
                                                               uint64_t *cxt,
                                                               int (*sliceTask)(uint64_t *, uint64_t, uint64_t, uint64_t),
                                                               uint64_t startIter, uint64_t maxIter) {
  rc = loop_handler_level1(cxt, sliceTask, startIter, maxIter);
  rollbackward
}

#endif