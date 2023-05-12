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

#ifndef SMALLEST_GRANULARITY
  #error "Macro SMALLEST_GRANULARITY undefined"
#endif

#define CACHELINE 8
#define START_ITER 0
#define MAX_ITER 1
#define LIVE_IN_ENV 2
#define LIVE_OUT_ENV 3
#define CHUNKSIZE 4
#define POLLING_COUNT 5

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

#if defined(DEBUG)
void printGIS(uint64_t *cxts, uint64_t startLevel, uint64_t maxLevel) {
  assert(startLevel <= maxLevel && startLevel >=0 && maxLevel >=0);
  for (uint64_t level = startLevel; level <= maxLevel; level++) {
    printf("level: %ld, [%ld, %ld)\n", level, cxts[level * CACHELINE + START_ITER], cxts[level * CACHELINE + MAX_ITER]);
  }
}
#endif

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
  return true;
}

#if !defined(ENABLE_ROLLFORWARD) && defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
#define TARGET_POLLING_RATIO 64
#define AGGRESSIVENESS 0.5
void adaptive_chunksize_control(uint64_t *cxts, uint64_t startingLevel, uint64_t numLevels) {
  uint64_t polling_count_total = 0;
  for (uint64_t level = startingLevel; level < numLevels; level++) {
    polling_count_total += cxts[level * CACHELINE + POLLING_COUNT];
  }
  printf("chunksize = %ld, polling total = %ld\n", cxts[(numLevels-1) * CACHELINE + CHUNKSIZE], polling_count_total);

  int64_t error_t = polling_count_total - TARGET_POLLING_RATIO;
  double kp = 1.0 / TARGET_POLLING_RATIO * AGGRESSIVENESS;
  double u_t = (double)error_t * kp;
  double new_chunksize = (1 + u_t) * cxts[(numLevels-1) * CACHELINE + CHUNKSIZE];
  printf("\terror = %ld\n\tu = %.2f\n\tnew_chunksize = %.2f\n", error_t, u_t, new_chunksize);
  cxts[(numLevels-1) * CACHELINE + CHUNKSIZE] = (uint64_t)new_chunksize > 0 ? (uint64_t)new_chunksize : 1;
}
#endif

int64_t loop_handler(
  uint64_t *cxts,
  uint64_t *constLiveIns,
  uint64_t startingLevel,
  uint64_t receivingLevel,
  uint64_t numLevels,
  int64_t (*slice_tasks[])(uint64_t *, uint64_t *, uint64_t, uint64_t),
  void (*leftover_tasks[])(uint64_t *, uint64_t *, uint64_t, uint64_t),
  uint64_t (*leftover_selector)(uint64_t, uint64_t)
) {
#if defined(DISABLE_HEARTBEAT_PROMOTION)
  return 0;
#endif
#if defined(STATS)
  heartbeats++;
#endif

  /*
   * Decide the splitting level
   */
  uint64_t splittingLevel = receivingLevel + 1;
  for (uint64_t level = startingLevel; level <= receivingLevel; level++) {
    if (cxts[level * CACHELINE + MAX_ITER] - cxts[level * CACHELINE + START_ITER] >= SMALLEST_GRANULARITY) {
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

#if !defined(ENABLE_ROLLFORWARD) && defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
  adaptive_chunksize_control(cxts, startingLevel, numLevels);
#endif

  /*
   * Calculate the splitting point of the rest of iterations at splittingLevel
   */
  uint64_t mid = (cxts[splittingLevel * CACHELINE + START_ITER] + 1 + cxts[splittingLevel * CACHELINE + MAX_ITER]) / 2;

  /*
   * Allocate the second context
   */
  uint64_t *cxtsSecond = (uint64_t *)__builtin_alloca(numLevels * CACHELINE * sizeof(uint64_t));

  /*
   * Construct the context at the splittingLevel for the second task
   */
  cxtsSecond[splittingLevel * CACHELINE + START_ITER]   = mid;
  cxtsSecond[splittingLevel * CACHELINE + MAX_ITER]     = cxts[splittingLevel * CACHELINE + MAX_ITER];
  cxtsSecond[splittingLevel * CACHELINE + LIVE_IN_ENV]  = cxts[splittingLevel * CACHELINE + LIVE_IN_ENV];
  cxtsSecond[splittingLevel * CACHELINE + LIVE_OUT_ENV] = cxts[splittingLevel * CACHELINE + LIVE_OUT_ENV];

#if defined(CHUNK_LOOP_ITERATIONS)
  /*
   * Copy the chunksize settings starting from splittingLevel
   */
  for (auto level = splittingLevel; level < numLevels; level++) {
    cxtsSecond[level * CACHELINE + CHUNKSIZE] = cxts[level * CACHELINE + CHUNKSIZE];
#if !defined(ENABLE_ROLLFORWARD) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
    /*
     * Reset polling count starting from splittingLevel
     */
    cxts[level * CACHELINE + POLLING_COUNT] = 0;
    cxtsSecond[level * CACHELINE + POLLING_COUNT] = 0;
#endif
  }
#endif

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
      slice_tasks[receivingLevel](cxts, constLiveIns, receivingLevel, 0);
    }, [&] {
      heartbeat_reset();
      slice_tasks[receivingLevel](cxtsSecond, constLiveIns, receivingLevel, 1);
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
      (*leftover_tasks[leftoverTaskIndex])(cxts, constLiveIns, splittingLevel, 0);
    }, [&] {
      heartbeat_reset();
      slice_tasks[splittingLevel](cxtsSecond, constLiveIns, splittingLevel, 1);
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
  uint64_t *constLiveIns,
  uint64_t startingLevel,
  uint64_t receivingLevel,
  uint64_t numLevels,
  int64_t (*slice_tasks[])(uint64_t *, uint64_t *, uint64_t, uint64_t),
  void (*leftover_tasks[])(uint64_t *, uint64_t *, uint64_t, uint64_t),
  uint64_t (*leftover_selector)(uint64_t, uint64_t)
) {
  rc = loop_handler(
    cxts, constLiveIns, startingLevel, receivingLevel, numLevels,
    slice_tasks, leftover_tasks, leftover_selector
  );
  rollbackward
}
#endif

}
