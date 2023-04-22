#include "loop_handler.hpp"
#if defined(STATS)
#include "utility.hpp"
#include <stdio.h>
#endif
#include <cstdint>
#include <functional>
#include <taskparts/benchmark.hpp>
#if defined(ENABLE_ROLLFORWARD)
#include <rollforward.h>
#endif

extern "C" {

#if defined(STATS)
static uint64_t polls = 0;
static uint64_t heartbeats = 0;
static uint64_t splits = 0;
#endif

void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end) {
#if defined(STATS)
  utility::stats_begin();
#endif

  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    bench_body();
  }, [&] (auto sched) {
    bench_start();
  }, [&] (auto sched) {
    bench_end();
  });

#if defined(STATS)
  utility::stats_end();
  printf("polls: %ld\n", polls);
  printf("heartbeats: %ld\n", heartbeats);
  printf("splits: %ld\n", splits);
#endif
}

__attribute__((always_inline))
void heartbeat_reset() {
#if !defined(ENABLE_ROLLFORWARD)
  taskparts::prev.mine() = taskparts::cycles::now();
#endif
}

bool heartbeat_polling() {
#if defined(STATS)
  polls++;
#endif
  if ((taskparts::prev.mine() + taskparts::kappa_cycles) > taskparts::cycles::now()) {
    return false;
  }
#if defined(STATS)
  heartbeats++;
#endif
  return true;
}

#ifndef SMALLEST_GRANULARITY
  #error "Macro SMALLEST_GRANULARITY undefined"
#endif

#define CACHELINE 8
#define START_ITER 0
#define MAX_ITER 1
#define LIVE_IN_ENV 2
#define LIVE_OUT_ENV 3

int64_t loop_handler(
  uint64_t *cxts,
  uint64_t receivingLevel,
  int64_t (*slice_tasks[])(uint64_t *, uint64_t),
  void (*leftover_tasks[])(uint64_t *, uint64_t),
  uint64_t (*leftover_selector)(uint64_t, uint64_t)
) {
#if defined(DISABLE_HEARTBEAT_PROMOTION)
  return 0;
#endif

  /*
   * Decide the splitting level
   */
  uint64_t splittingLevel = receivingLevel + 1;
  for (uint64_t level = 0; level <= receivingLevel; level++) {
    if (cxts[level * CACHELINE + MAX_ITER] - cxts[level * CACHELINE + START_ITER] >= (SMALLEST_GRANULARITY + 1)) {
      splittingLevel = level;
      break;
    }
  }
  if (splittingLevel > receivingLevel) {
    /*
     * No more work to split at any level
     */
    return 0;
  }
#if defined(STATS)
  splits++;
#endif

  /*
   * Calculate the splitting point of the rest of iterations at splittingLevel
   */
  uint64_t mid = (cxts[splittingLevel * CACHELINE + START_ITER] + 1 + cxts[splittingLevel * CACHELINE + MAX_ITER]) / 2;

  /*
   * Allocate and construct the context at the splittingLevel for the second task
   */
  uint64_t cxtsSecond[2 * CACHELINE];
  cxtsSecond[splittingLevel * CACHELINE + START_ITER]   = mid;
  cxtsSecond[splittingLevel * CACHELINE + MAX_ITER]     = cxts[splittingLevel * CACHELINE + MAX_ITER];
  cxtsSecond[splittingLevel * CACHELINE + LIVE_IN_ENV]  = cxts[splittingLevel * CACHELINE + LIVE_IN_ENV];
  cxtsSecond[splittingLevel * CACHELINE + LIVE_OUT_ENV] = cxts[splittingLevel * CACHELINE + LIVE_OUT_ENV];

  /*
   * Set the maxIteration for the first task
   */
  cxts[splittingLevel * CACHELINE + MAX_ITER] = mid;

  if (splittingLevel == receivingLevel) { // no leftover task needed
    /*
     * First task starts from the next iteration
     */
    cxts[receivingLevel * CACHELINE + START_ITER]++;

    taskparts::tpalrts_promote_via_nativefj([&] {
      heartbeat_reset();
      slice_tasks[receivingLevel](cxts, 0);
    }, [&] {
      heartbeat_reset();
      slice_tasks[receivingLevel](cxtsSecond, 1);
    }, [] { }, taskparts::bench_scheduler());
  
  } else { // the first task needs to compose the leftover work

    /*
     * Set the startIter for the leftover work to start from
     */
    for (uint64_t level = splittingLevel + 1; level <= receivingLevel; level++) {
      cxts[level * CACHELINE + START_ITER]++;
    }

    /*
     * Determine which leftover task to run
     */
    uint64_t leftoverTaskIndex = leftover_selector(receivingLevel, splittingLevel);
    taskparts::tpalrts_promote_via_nativefj([&] {
      heartbeat_reset();
      (*leftover_tasks[leftoverTaskIndex])(cxts, 0);
    }, [&] {
      heartbeat_reset();
      slice_tasks[splittingLevel](cxtsSecond, 1);
    }, [&] { }, taskparts::bench_scheduler());
  }

  /*
   * Return the number of levels that maxIter needs to be reset
   */
  return receivingLevel - splittingLevel + 1;
}

#if defined(ENABLE_ROLLFORWARD)
void rollforward_handler_annotation __rf_handle_wrapper(
  int64_t &rc,
  uint64_t *cxts,
  uint64_t receivingLevel,
  int64_t (*slice_tasks[])(uint64_t *, uint64_t),
  void (*leftover_tasks[])(uint64_t *, uint64_t),
  uint64_t (*leftover_selector)(uint64_t, uint64_t)
) {
  rc = loop_handler(
    cxts, receivingLevel,
    slice_tasks, leftover_tasks, leftover_selector
  );
  rollbackward
}
#endif

}
