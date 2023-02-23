#pragma once

#include <functional>
#include <taskparts/benchmark.hpp>
#include <alloca.h>

#define CACHELINE         8
#define LIVE_IN_ENV       0
#define LIVE_OUT_ENV      1
#define START_ITER        0
#define MAX_ITER          1

#ifndef SMALLEST_GRANULARITY
  #error "Macro SMALLEST_GRANULARITY undefined"
#endif

void loop_dispatcher(std::function<void()> const& lambda) {
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    lambda();
  });

  return;
}

#if defined(HEARTBEAT_BRANCHES)

#elif defined(HEARTBEAT_VERSIONING)

#define TWO_LEVELS 2
int64_t loop_handler_level2(
  uint64_t *cxts,
  uint64_t receivingLevel,
  int64_t(*slice_tasks[])(uint64_t * /* cxts */, uint64_t /* myIndex */, uint64_t /* startIter */, uint64_t /* maxIter */),
  int64_t(*leftover_tasks[])(uint64_t * /* cxts */, uint64_t /* myIndex */, uint64_t * /* itersArr */),
  uint64_t(*leftover_selector)(uint64_t /* splittingLevel */, uint64_t /* receivingLevel */),
  uint64_t s0, uint64_t m0,
  uint64_t s1, uint64_t m1
) {
#if defined(DISABLE_HEARTBEAT_PROMOTION)
  return 0;
#endif

  /*
   * Determine whether to promote since last promotion
   */
  auto &p = taskparts::prev.mine();
  auto n = taskparts::cycles::now();
  if ((p + taskparts::kappa_cycles) > n) {
    return 0;
  }
  p = n;

  /*
   * Decide the splitting level
   */
  uint64_t splittingLevel = TWO_LEVELS;
  if ((m0 - s0) >= (SMALLEST_GRANULARITY + 1)) {
    splittingLevel = 0;
  } else if ((m1 - s1) >= (SMALLEST_GRANULARITY + 1)) {
    splittingLevel = 1;
  }

  /*
   * No more work to split at any level
   */
  if (splittingLevel == TWO_LEVELS) {
    return 0;
  }

  /*
   * Snapshot global iteration state
   */
  uint64_t *itersArr = (uint64_t *)alloca(sizeof(uint64_t) * TWO_LEVELS * 2);
  uint64_t index = 0;
  itersArr[index++] = s0;
  itersArr[index++] = m0;
  itersArr[index++] = s1;
  itersArr[index++] = m1;

  /*
   * Allocate cxts for both tasks
   */
  uint64_t *cxtsFirst   = (uint64_t *)alloca(sizeof(uint64_t) * TWO_LEVELS * CACHELINE);
  uint64_t *cxtsSecond  = (uint64_t *)alloca(sizeof(uint64_t) * TWO_LEVELS * CACHELINE);

  /*
   * Determine the splitting point of the remaining iterations
   */
  uint64_t med = (itersArr[splittingLevel * 2 + START_ITER] + 1 + itersArr[splittingLevel * 2 + MAX_ITER]) / 2;

  /*
   * Reconstruct the context at the splittingLevel for both tasks
   */
  cxtsFirst[splittingLevel * CACHELINE + LIVE_IN_ENV]   = cxts[splittingLevel * CACHELINE + LIVE_IN_ENV];
  cxtsSecond[splittingLevel * CACHELINE + LIVE_IN_ENV]  = cxts[splittingLevel * CACHELINE + LIVE_IN_ENV];
  cxtsFirst[splittingLevel * CACHELINE + LIVE_OUT_ENV]  = cxts[splittingLevel * CACHELINE + LIVE_OUT_ENV];
  cxtsSecond[splittingLevel * CACHELINE + LIVE_OUT_ENV] = cxts[splittingLevel * CACHELINE + LIVE_OUT_ENV];

  if (splittingLevel == receivingLevel) { // no leftover task needed
      taskparts::tpalrts_promote_via_nativefj([&] {
        slice_tasks[splittingLevel](cxtsFirst, 0, itersArr[receivingLevel * 2 + START_ITER]+1, med);
      }, [&] {
        slice_tasks[splittingLevel](cxtsSecond, 1, med, itersArr[receivingLevel * 2 + MAX_ITER]);
      }, [] { }, taskparts::bench_scheduler());

  } else { // build up the leftover task
    /*
     * Allocate context for leftover task
     */
    uint64_t *cxtsLeftover = (uint64_t *)alloca(sizeof(uint64_t) * TWO_LEVELS * CACHELINE);

    /*
     * Reconstruct the context starting from splittingLevel + 1 up to receivingLevel for leftover task
     */
    for (uint64_t level = splittingLevel + 1; level <= receivingLevel; level++) {
      uint64_t index = level * CACHELINE;
      cxtsLeftover[index + LIVE_IN_ENV]   = cxts[index + LIVE_IN_ENV];
      cxtsLeftover[index + LIVE_OUT_ENV]  = cxts[index + LIVE_OUT_ENV];

      /*
       * Rreset global iters for leftover task
       */
      itersArr[level * 2 + START_ITER] += 1;
    }

    /*
     * Determine which leftover task to run
     */
    uint64_t leftoverTaskIndex = leftover_selector(splittingLevel, receivingLevel);

    taskparts::tpalrts_promote_via_nativefj([&] {
      (*leftover_tasks[leftoverTaskIndex])(cxtsLeftover, 0, itersArr);
    }, [&] {
      taskparts::tpalrts_promote_via_nativefj([&] {
        slice_tasks[splittingLevel](cxtsFirst, 0, itersArr[splittingLevel * 2 + START_ITER]+1, med);
      }, [&] {
        slice_tasks[splittingLevel](cxtsSecond, 1, med, itersArr[splittingLevel * 2 + MAX_ITER]);
      }, [] { }, taskparts::bench_scheduler());
    }, [&] { }, taskparts::bench_scheduler());
  }

  /*
   * Return the number of levels that maxIter needs to be reset
   */
  return receivingLevel - splittingLevel + 1;
}

#endif
