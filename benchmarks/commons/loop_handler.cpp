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

#if !defined(SMALLEST_GRANULARITY)
  #error "Macro SMALLEST_GRANULARITY no defined"
#endif

#define CACHELINE 8
#define START_ITER 0
#define MAX_ITER 1
#define LIVE_IN_ENV 2
#define LIVE_OUT_ENV 3
#define CHUNKSIZE 4

#if defined(STATS)
static uint64_t polls = 0;
#if defined(POLLS_STATS)
static uint64_t prev_polls = 0;
#endif
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

void printGIS(uint64_t *cxts, uint64_t startLevel, uint64_t maxLevel, std::string prefix) {
  assert(startLevel <= maxLevel && startLevel >=0 && maxLevel >=0);
  for (uint64_t level = startLevel; level <= maxLevel; level++) {
    printf("%slevel: %ld, [%ld, %ld)\n", prefix.c_str(), level, cxts[level * CACHELINE + START_ITER], cxts[level * CACHELINE + MAX_ITER]);
  }
}

#if !defined(ENABLE_ROLLFORWARD)

bool heartbeat_polling(task_memory_t *tmem) {
#if defined(STATS)
  polls++;
#endif

#if defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
  /*
   * Increase the polling count in the task memory
   */
  tmem->polling_count++;
#endif
  if ((taskparts::prev.mine() + taskparts::kappa_cycles) > taskparts::cycles::now()) {
    return false;
  }
  return true;
}

#if defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
/*
 * Memorization information tracked by the runtime worker thread and
 * accessed and analyzed by the runtime functions
 */
struct runtime_memory_t {
  uint64_t heartbeat_count;
  uint64_t minimal_polling_count;
  uint64_t chunksize;
};

taskparts::perworker::array<runtime_memory_t> rtmem;

void runtime_memory_reset() {
  for (size_t i = 0; i < taskparts::perworker::nb_workers(); i++) {
    rtmem[i].heartbeat_count = 0;
    rtmem[i].minimal_polling_count = INT64_MAX;
    rtmem[i].chunksize = 1;
  }
}

#define SLIDING_WINDOW_SIZE 5
#define TARGET_POLLING_RATIO 2
#define AGGRESSIVENESS 1
/*
 * Function invoked inside the loop_handler (when received a heartbeat),
 * to update the memory tracked by the runtime thread
 */
__attribute__((always_inline))
void runtime_memory_update(task_memory_t *tmem, uint64_t *cxts, uint64_t numLevels) {
  /*
   * Increase the heartbeat count for this thread
   */
  rtmem.mine().heartbeat_count++;

  /*
   * Update the minimal polling count for this window,
   * as well as the chunksize used for this new polling count
   */
  if (tmem->polling_count < rtmem.mine().minimal_polling_count) {
    rtmem.mine().minimal_polling_count = tmem->polling_count;
    rtmem.mine().chunksize = cxts[(numLevels-1) * CACHELINE + CHUNKSIZE];
  }

#if defined(ACC_STATS)
  printf("runtime_memory_update: ");
  printf("heartbeat_count = %ld, ", rtmem.mine().heartbeat_count);
  printf("polling_count = %ld, ", tmem->polling_count);
  printf("minimal_polling_count = %ld, ", rtmem.mine().minimal_polling_count);
  printf("chunksize = %ld\n", rtmem.mine().chunksize);
#endif

  /*
   * Do adaptive chunksize control once finishes a window
   */
  if (rtmem.mine().heartbeat_count % SLIDING_WINDOW_SIZE == 0) {
    /*
     * Adaptive chunksize control algorithm
     */
    double u_t = (double)rtmem.mine().minimal_polling_count / (double)TARGET_POLLING_RATIO * (double)AGGRESSIVENESS;
    double new_chunksize = u_t * rtmem.mine().chunksize;
  #if defined(ACC_STATS)
    printf("\tapplying adpative chunksize control:\n");
    printf("\t\tu = %.2f\n", u_t);
    printf("\t\told chunksize = %ld\n", (uint64_t)rtmem.mine().chunksize);
    printf("\t\tnew chunksize = %ld\n", (uint64_t)new_chunksize);
  #endif
    /*
    * Update the new chunksize to the runtime memory, so whichever
    * task run by this thread can inherit the new chunksize setting
    */
    rtmem.mine().chunksize = (uint64_t)new_chunksize > 0 ? (uint64_t)new_chunksize <= INT32_MAX ? (uint64_t)new_chunksize : INT32_MAX : 1;

    /*
     * Reset minimal polling count
     */
    rtmem.mine().minimal_polling_count = INT64_MAX;
  }

  return;
}

#endif  // defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
#endif  // !defined(ENABLE_ROLLFORWARD)

__attribute__((always_inline))
void task_memory_reset(task_memory_t *tmem, uint64_t startingLevel) {
  /*
   * Set the starting level
   */
  tmem->startingLevel = startingLevel;

#if !defined(ENABLE_ROLLFORWARD)
#if defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
  /*
   * Reset polling count if using adaptive chunksize control
   */
  tmem->polling_count = 0;
#endif
  /*
   * Reset heartbeat timer if using software polling
   */
  taskparts::prev.mine() = taskparts::cycles::now();
#endif
}

void heartbeat_start(task_memory_t *tmem) {
#if !defined(ENABLE_ROLLFORWARD) && defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
  runtime_memory_reset();
#endif
  task_memory_reset(tmem, 0);
}

int64_t loop_handler(
  uint64_t *cxts,
  uint64_t *constLiveIns,
  uint64_t receivingLevel,
  uint64_t numLevels,
  task_memory_t *tmem,
  int64_t (*slice_tasks[])(uint64_t *, uint64_t *, uint64_t, task_memory_t *),
  void (*leftover_tasks[])(uint64_t *, uint64_t *, uint64_t, task_memory_t *),
  uint64_t (*leftover_selector)(uint64_t, uint64_t)
) {
#if defined(STATS)
  heartbeats++;
#if defined(POLLS_STATS)
  printf("%ld\n", polls-prev_polls);
  prev_polls = polls;
#endif
#endif
#if defined(DISABLE_HEARTBEAT_PROMOTION)
  return 0;
#endif

  /*
   * Decide the splitting level
   */
  uint64_t splittingLevel = receivingLevel + 1;
  for (uint64_t level = tmem->startingLevel; level <= receivingLevel; level++) {
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
  runtime_memory_update(tmem, cxts, numLevels);
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
#if !defined(ENABLE_ROLLFORWARD) && defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
      cxts[(numLevels-1) * CACHELINE + CHUNKSIZE] = rtmem.mine().chunksize;
#endif
      task_memory_reset(tmem, receivingLevel);
      slice_tasks[receivingLevel](cxts, constLiveIns, 0, tmem);
    }, [&] {
#if !defined(ENABLE_ROLLFORWARD) && defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
      cxtsSecond[(numLevels-1) * CACHELINE + CHUNKSIZE] = rtmem.mine().chunksize;
#endif
      task_memory_t hbmemSecond;
      task_memory_reset(&hbmemSecond, receivingLevel);
      slice_tasks[receivingLevel](cxtsSecond, constLiveIns, 1, &hbmemSecond);
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
#if !defined(ENABLE_ROLLFORWARD) && defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
      cxts[(numLevels-1) * CACHELINE + CHUNKSIZE] = rtmem.mine().chunksize;
#endif
      task_memory_reset(tmem, splittingLevel);
      (*leftover_tasks[leftoverTaskIndex])(cxts, constLiveIns, 0, tmem);
    }, [&] {
#if !defined(ENABLE_ROLLFORWARD) && defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
      cxtsSecond[(numLevels-1) * CACHELINE + CHUNKSIZE] = rtmem.mine().chunksize;
#endif
      task_memory_t hbmemSecond;
      task_memory_reset(&hbmemSecond, splittingLevel);
      slice_tasks[splittingLevel](cxtsSecond, constLiveIns, 1, &hbmemSecond);
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
  uint64_t receivingLevel,
  uint64_t numLevels,
  task_memory_t *tmem,
  int64_t (*slice_tasks[])(uint64_t *, uint64_t *, uint64_t, task_memory_t *),
  void (*leftover_tasks[])(uint64_t *, uint64_t *, uint64_t, task_memory_t *),
  uint64_t (*leftover_selector)(uint64_t, uint64_t)
) {
  rc = loop_handler(
    cxts, constLiveIns, receivingLevel, numLevels, tmem,
    slice_tasks, leftover_tasks, leftover_selector
  );
  rollbackward
}
#endif

}
