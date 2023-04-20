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

bool heartbeat_polling() {
#if defined(STATS)
  polls++;
#endif
  auto &p = taskparts::prev.mine();
  auto n = taskparts::cycles::now();
  if ((p + taskparts::kappa_cycles) > n) {
    return false;
  }
  p = n;
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
      slice_tasks[receivingLevel](cxts, 0);
    }, [&] {
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
      (*leftover_tasks[leftoverTaskIndex])(cxts, 0);
    }, [&] {
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
  rc = loop_handler_level2(
    cxts, receivingLevel,
    slice_tasks, leftover_tasks, leftover_selector
  );
  rollbackward
}
#endif

// int64_t loop_handler_level1(
//   void *cxt,
//   int64_t (*sliceTask)(void *, uint64_t, uint64_t, uint64_t),
//   uint64_t startIter, uint64_t maxIter
// ) {
// #if defined(DISABLE_HEARTBEAT_PROMOTION)
//   return 0;
// #endif

//   /*
//    * Determine if there's more work for splitting
//    */
//   if ((maxIter - startIter) <= SMALLEST_GRANULARITY) {
//     return 0;
//   }
// #if defined(STATS)
//   splits++;
// #endif

//   /*
//    * Calculate the splitting point of the rest of iterations
//    */
//   uint64_t mid = (startIter + 1 + maxIter) / 2;

//   /*
//    * Allocate cxts for the second task
//    */
//   uint64_t cxtSecond[1 * CACHELINE];

//   /*
//    * Construct cxts for the second task
//    */
//   cxtSecond[LIVE_IN_ENV]  = ((uint64_t *)cxt)[LIVE_IN_ENV];
//   cxtSecond[LIVE_OUT_ENV] = ((uint64_t *)cxt)[LIVE_OUT_ENV];

//   /*
//    * Invoke splitting tasks
//    */
//   taskparts::tpalrts_promote_via_nativefj([&] {
//     (*sliceTask)((void *)cxt, 0, startIter + 1, mid);
//   }, [&] {
//     (*sliceTask)((void *)cxtSecond, 1, mid, maxIter);
//   }, [] { }, taskparts::bench_scheduler());

//   return 1;
// }

// #if defined(ENABLE_ROLLFORWARD)
// void rollforward_handler_annotation __rf_handle_level1_wrapper(
//   int64_t &rc,
//   void *cxt,
//   int64_t (*sliceTask)(void *, uint64_t, uint64_t, uint64_t),
//   uint64_t startIter, uint64_t maxIter
// ) {
//   rc = loop_handler_level1(cxt, sliceTask, startIter, maxIter);
//   rollbackward
// }
// #endif

// int64_t loop_handler_level2(
//   void *cxts,
//   uint64_t receivingLevel,
//   void (*slice_tasks[])(void *, uint64_t, uint64_t, uint64_t),
//   void (*leftover_tasks[])(void *, uint64_t, void *),
//   uint64_t (*leftover_selector)(uint64_t, uint64_t),
//   uint64_t s0, uint64_t m0,
//   uint64_t s1, uint64_t m1
// ) {
// #if defined(DISABLE_HEARTBEAT_PROMOTION)
//   return 0;
// #endif

//   /*
//    * Decide the splitting level
//    */
//   uint64_t splittingLevel = 2;
//   if ((m0 - s0) >= (SMALLEST_GRANULARITY + 1)) {
//     splittingLevel = 0;
//   } else if ((m1 - s1) >= (SMALLEST_GRANULARITY + 1)) {
//     splittingLevel = 1;
//   } else {
//     /*
//      * No more work to split at any level
//      */
//     return 0;
//   }
// #if defined(STATS)
//   splits++;
// #endif

//   /*
//    * Snapshot global iteration state
//    */
//   uint64_t itersArr[2 * 2];
//   uint64_t index = 0;
//   itersArr[index++] = s0;
//   itersArr[index++] = m0;
//   itersArr[index++] = s1;
//   itersArr[index++] = m1;

//   /*
//    * Calculate the splitting point of the rest of iterations at splittingLevel
//    */
//   uint64_t mid = (itersArr[splittingLevel * 2 + START_ITER] + 1 + itersArr[splittingLevel * 2 + MAX_ITER]) / 2;

//   /*
//    * Allocate cxts for the second task
//    */
//   uint64_t cxtsSecond[2 * CACHELINE];

//   /*
//    * Construct the context at the splittingLevel for the second task
//    */
//   cxtsSecond[splittingLevel * CACHELINE + LIVE_IN_ENV]  = ((uint64_t *)cxts)[splittingLevel * CACHELINE + LIVE_IN_ENV];
//   cxtsSecond[splittingLevel * CACHELINE + LIVE_OUT_ENV] = ((uint64_t *)cxts)[splittingLevel * CACHELINE + LIVE_OUT_ENV];

//   if (splittingLevel == receivingLevel) { // no leftover task needed
//     taskparts::tpalrts_promote_via_nativefj([&] {
//       slice_tasks[receivingLevel]((void *)cxts, 0, itersArr[receivingLevel * 2 + START_ITER]+1, mid);
//     }, [&] {
//       slice_tasks[receivingLevel]((void *)cxtsSecond, 1, mid, itersArr[receivingLevel * 2 + MAX_ITER]);
//     }, [] { }, taskparts::bench_scheduler());
  
//   } else { // the first task needs to compose the leftover work

//     /*
//      * Set the startIter in itersArr for the leftover work to start from
//      */
//     for (uint64_t level = splittingLevel + 1; level <= receivingLevel; level++) {
//       itersArr[level * 2 + START_ITER] += 1;
//     }

//     /*
//      * Get the maxIter used by the second task
//      */
//     uint64_t max = itersArr[splittingLevel * 2 + MAX_ITER];

//     /*
//      * Set the maxIter in itersArr at the splittingLevel for the leftover task
//      */
//     itersArr[splittingLevel * 2 + MAX_ITER] = mid;

//     /*
//      * Determine which leftover task to run
//      */
//     uint64_t leftoverTaskIndex = leftover_selector(receivingLevel, splittingLevel);
//     taskparts::tpalrts_promote_via_nativefj([&] {
//       (*leftover_tasks[leftoverTaskIndex])((void *)cxts, 0, (void *)itersArr);
//     }, [&] {
//       slice_tasks[splittingLevel]((void *)cxtsSecond, 1, mid, max);
//     }, [&] { }, taskparts::bench_scheduler());
//   }

//   /*
//    * Return the number of levels that maxIter needs to be reset
//    */
//   return receivingLevel - splittingLevel + 1;
// }

// #if defined(ENABLE_ROLLFORWARD)
// void rollforward_handler_annotation __rf_handle_level2_wrapper(
//   int64_t &rc,
//   void *cxts,
//   uint64_t receivingLevel,
//   void (*slice_tasks[])(void *, uint64_t, uint64_t, uint64_t),
//   void (*leftover_tasks[])(void *, uint64_t, void *),
//   uint64_t (*leftover_selector)(uint64_t, uint64_t),
//   uint64_t s0, uint64_t m0,
//   uint64_t s1, uint64_t m1
// ) {
//   rc = loop_handler_level2(
//     cxts, receivingLevel,
//     slice_tasks, leftover_tasks, leftover_selector,
//     s0, m0, s1, m1
//   );
//   rollbackward
// }
// #endif

}
