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

#define CACHELINE 8
#define START_ITER 0
#define MAX_ITER 1
#define LIVE_IN_ENV 2
#define LIVE_OUT_ENV 3

#if defined(STATS) || defined(POLLS_STATS) || defined(NUM_POLLS_STATS) || defined(BEATS_STATS)
static uint64_t polls = 0;
static uint64_t heartbeats = 0;
#if defined(STATS)
static uint64_t splits = 0;
#endif
#if defined(POLLS_STATS)
static uint64_t prev_polls = 0;
#endif
#endif

#if defined(PROMO_STATS)
static int maxLevel = 3;
std::unordered_map<int, uint64_t> levelCountMap;
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
#if defined(NUM_POLLS_STATS)
  printf("polls: %ld\n", polls);
#endif
#if defined(BEATS_STATS)
  printf("heartbeats: %ld\n", heartbeats);
#endif
#if defined(PROMO_STATS)
  for (auto i = 0; i < maxLevel; i++) {
    printf("%d\t%lu\n", i, levelCountMap[i]);
  }
#endif
}

void printGIS(uint64_t *cxts, uint64_t startLevel, uint64_t maxLevel, std::string prefix) {
  assert(startLevel <= maxLevel && startLevel >=0 && maxLevel >=0);
  for (uint64_t level = startLevel; level <= maxLevel; level++) {
    printf("%slevel: %ld, [%ld, %ld)\n", prefix.c_str(), level, cxts[level * CACHELINE + START_ITER], cxts[level * CACHELINE + MAX_ITER]);
  }
}

#if defined(ENABLE_SOFTWARE_POLLING)

thread_local uint64_t timestamp = 0;

bool heartbeat_polling(task_memory_t *tmem) {
#if defined(STATS) || defined(POLLS_STATS) || defined(NUM_POLLS_STATS) || defined(BEATS_STATS)
  polls++;
#endif

#if defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
  /*
   * Increase the polling count in the task memory
   */
  tmem->polling_count++;
#endif
  if ((timestamp + taskparts::kappa_cycles) > taskparts::cycles::now()) {
    return false;
  }
  return true;
}

#if defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
/*
 * Memorization information tracked by the runtime worker thread and
 * accessed and analyzed by the runtime functions
 */
thread_local uint64_t heartbeat_count = 0;
#if defined(ACC_MINIMAL)
thread_local uint64_t polling_count = INT64_MAX;
#elif defined(ACC_MAXIMAL)
thread_local uint64_t polling_count = 0;
#endif
thread_local uint64_t chunksize = CHUNKSIZE;
#if defined(ACC_EVAL)
thread_local uint64_t polling_count_last_window = 0;
thread_local uint64_t success_count = 0;
#endif

thread_local uint64_t sliding_window_size = 5;
thread_local uint64_t target_polling_ratio = 8;
#if defined(ACC_SPMV_STATS)
typedef struct {
  uint64_t startIter;
  uint64_t chunksize;
} ass_t;
ass_t *ass_stats = nullptr;
uint64_t ass_begin = 1;
static uint64_t ass_max = 1000;
#endif

void runtime_memory_reset() {
#if defined(ACC_SPMV_STATS)
  ass_stats = (ass_t *)malloc(ass_max * sizeof(ass_t));
  ass_stats[0].startIter = 0;
  ass_stats[0].chunksize = CHUNKSIZE;
#endif

  if (const char *s = std::getenv("SLIDING_WINDOW_SIZE")) {
    sliding_window_size = std::atoll(s);
  }
  if (const char *s = std::getenv("TARGET_POLLING_RATIO")) {
    target_polling_ratio = std::atoll(s);
  }
}

/*
 * Function invoked inside the loop_handler (when received a heartbeat),
 * to update the memory tracked by the runtime thread
 */
__attribute__((always_inline))
void runtime_memory_update(task_memory_t *tmem, uint64_t *cxts, uint64_t numLevels) {
  /*
   * Increase the heartbeat count for this thread
   */
  heartbeat_count++;

  /*
   * Update the polling count for this window,
   */
#if defined(ACC_MINIMAL)
  if (tmem->polling_count < polling_count) {
    polling_count = tmem->polling_count;
  }
#elif defined(ACC_MAXIMAL)
  if (tmem->polling_count > polling_count) {
    polling_count = tmem->polling_count;
  }
#endif

#if defined(ACC_DEBUG)
  printf("runtime_memory_update: ");
  printf("heartbeat_count = %ld, ", heartbeat_count);
  printf("polling_count = %ld, ", tmem->polling_count);
  printf("target_polling_count = %ld, ", polling_count);
  printf("chunksize = %ld\n", chunksize);
#endif

#if defined(ACC_EVAL)
  // When evaluating ACC, we do it conservatively to stay faithful to heartbeat scheduling
  if (polling_count_last_window <= polling_count) {
    success_count++;
  }
#endif

  /*
   * Do adaptive chunksize control once finishes a window
   */
  if (heartbeat_count % sliding_window_size == 0) {
    /*
     * Adaptive chunksize control algorithm
     */
    double u_t = (double)polling_count / (double)target_polling_ratio;
    double new_chunksize = u_t * chunksize;
#if defined(ACC_DEBUG)
    printf("\tapplying adpative chunksize control:\n");
    printf("\t\tu = %.2f\n", u_t);
    printf("\t\told chunksize = %ld\n", (uint64_t)chunksize);
    printf("\t\tnew chunksize = %ld\n", (uint64_t)new_chunksize);
#endif
    /*
     * Update the new chunksize to the runtime memory, so whichever
     * task run by this thread can inherit the new chunksize setting
     */
#if !defined(ACC_EVAL)
    chunksize = (uint64_t)new_chunksize > 0 ? (uint64_t)new_chunksize : 1;
#else
    polling_count_last_window = (uint64_t)((double)polling_count / aggressiveness);
    printf("%.2f\n", (double)success_count / (double)heartbeat_count * 100);
#endif

    /*
     * Reset polling count
     */
#if defined(ACC_MINIMAL)
    polling_count = INT64_MAX;
#elif defined(ACC_MAXIMAL)
    polling_count = 0;
#endif
  }

  return;
}

#if defined(ACC_SPMV_STATS)
void ass_record(uint64_t startIter) {
  ass_stats[ass_begin].startIter = startIter;
  ass_stats[ass_begin].chunksize = chunksize;
  ass_begin++;
  assert(ass_begin < ass_max);
}
#endif

#endif  // defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
#endif  // defined(ENABLE_SOFTWARE_POLLING)

#if defined(CHUNK_LOOP_ITERATIONS) && !defined(ADAPTIVE_CHUNKSIZE_CONTROL)
  static uint64_t chunksize = CHUNKSIZE;
#endif

__attribute__((always_inline))
void task_memory_reset(task_memory_t *tmem, uint64_t startingLevel) {
  /*
   * Set the starting level
   */
  tmem->startingLevel = startingLevel;

#if defined(ENABLE_SOFTWARE_POLLING)
  /*
   * To use acc one must use software polling and enable loop chunking
   */
#if defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
  /*
   * Reset polling count if using adaptive chunksize control
   */
  tmem->polling_count = 0;

  /*
   * Use the chunksize tracked by the runtime thread
   */
  tmem->chunksize = chunksize;
  tmem->remaining_chunksize = tmem->chunksize;
#elif defined(CHUNK_LOOP_ITERATIONS) && !defined(ADAPTIVE_CHUNKSIZE_CONTROL)
  /*
   * Use static chunksize
   */
  tmem->chunksize = chunksize;
  tmem->remaining_chunksize = chunksize;
#endif
  /*
   * Reset heartbeat timer if using software polling
   */
  timestamp = taskparts::cycles::now();
#else // ENABLE_ROLLFORWARD
#if defined(CHUNK_LOOP_ITERATIONS)
  /*
   * Use static chunksize
   */
  tmem->chunksize = chunksize;
  tmem->remaining_chunksize = chunksize;
#endif
#endif
}

#if defined(CHUNK_LOOP_ITERATIONS)
uint64_t get_chunksize(task_memory_t *tmem) {
  return tmem->remaining_chunksize;
}

bool update_and_has_remaining_chunksize(task_memory_t *tmem, uint64_t iterations, uint64_t chunksize) {
  if (iterations == chunksize) {
    tmem->remaining_chunksize = tmem->chunksize;
    return false;
  } else {
    tmem->remaining_chunksize -= iterations;
    return true;
  }
}
#endif

void heartbeat_start(task_memory_t *tmem) {
#if defined(ENABLE_HEARTBEAT)
#if defined(ENABLE_SOFTWARE_POLLING) && defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
  runtime_memory_reset();
#endif
#if defined(CHUNK_LOOP_ITERATIONS) && !defined(ADAPTIVE_CHUNKSIZE_CONTROL)
  if (const char *s = std::getenv("CHUNKSIZE")) {
    chunksize = std::atoll(s);
  }
#endif
#if defined(PROMO_STATS)
  for (auto i = 0; i < maxLevel; i++) {
    levelCountMap[i] = 0;
  }
#endif
  task_memory_reset(tmem, 0);
#endif
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
#if defined(STATS) || defined(BEATS_STATS)
  heartbeats++;
#if !defined(ADAPTIVE_CHUNKSIZE_CONTROL)
  assert(polls >= 2);
#endif
  polls = 0;
#endif
#if defined(POLLS_STATS)
  printf("%ld\n", polls-prev_polls);
  prev_polls = polls;
#endif
#if defined(OVERHEAD_ANALYSIS)
#if defined(ENABLE_SOFTWARE_POLLING) && defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
  runtime_memory_update(tmem, cxts, numLevels);
#endif
  task_memory_reset(tmem, 0);
#endif
#if defined(DISABLE_PROMOTION)
  return 0;
#endif

  /*
   * Decide the splitting level
   */
  uint64_t splittingLevel = receivingLevel + 1;
  for (uint64_t level = tmem->startingLevel; level <= receivingLevel; level++) {
    if (cxts[level * CACHELINE + MAX_ITER] - cxts[level * CACHELINE + START_ITER] >= 2) {
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
#if defined(PROMO_STATS)
  levelCountMap[splittingLevel]++;
#endif

#if defined(ENABLE_SOFTWARE_POLLING) && defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
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
#if defined(ENABLE_SOFTWARE_POLLING) && defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL) && defined(ACC_SPMV_STATS)
      ass_record(cxts[0]);
#endif
      task_memory_reset(tmem, receivingLevel);
      slice_tasks[receivingLevel](cxts, constLiveIns, 0, tmem);
    }, [&] {
#if defined(ENABLE_SOFTWARE_POLLING) && defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL) && defined(ACC_SPMV_STATS)
      ass_record(cxtsSecond[0]);
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
      task_memory_reset(tmem, splittingLevel);
      (*leftover_tasks[leftoverTaskIndex])(cxts, constLiveIns, 0, tmem);
    }, [&] {
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
