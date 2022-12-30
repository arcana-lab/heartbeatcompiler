#pragma once

#include <taskparts/benchmark.hpp>
#include <alloca.h>

#ifndef SMALLEST_GRANULARITY
  #error "Macro SMALLEST_GRANULARITY undefined"
#endif

#if defined(COLLECT_HEARTBEAT_POLLING_TIME) || defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
#include <pthread.h>
#include <stdio.h>

#define RDTSC(rdtsc_val_) do { \
  uint32_t rdtsc_hi_, rdtsc_lo_; \
  __asm__ volatile ("rdtsc" : "=a" (rdtsc_lo_), "=d" (rdtsc_hi_)); \
  rdtsc_val_ = (uint64_t)rdtsc_hi_ << 32 | rdtsc_lo_; \
} while (0)

auto cpu_frequency_khz = taskparts::get_cpu_frequency_khz();
auto num_workers = taskparts::perworker::id::get_nb_workers();

#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
volatile pthread_spinlock_t lock;
static volatile uint64_t useful_cycles = 0;
static volatile uint64_t useful_invocations = 0;
static volatile uint64_t wasted_cycles = 0;
static volatile uint64_t wasted_invocations = 0;

void collect_heartbeat_polling_time_init() {
  pthread_spin_init(&lock, 0);
}

void collect_heartbeat_polling_time_print() {
  printf("========= Heartbeat Polling Time Summary =========\n");
  printf("cpu_frequency_khz: %lu\n", cpu_frequency_khz);
  printf("num_workers: %zu\n", num_workers);

  printf("\n");

  printf("useful_cycles: %lu\n", useful_cycles);
  printf("useful_invocations: %lu\n", useful_invocations);
  printf("useful_cycles_per_invocation: %.2f\n", useful_invocations == 0 ? 0.00 : (double)useful_cycles/(double)useful_invocations);
  printf("useful_seconds: %.2f\n", useful_invocations == 0 ? 0.00 : (double)useful_cycles/((double)cpu_frequency_khz * 1000)/(double)num_workers);

  printf("\n");

  printf("wasted_cycles: %lu\n", wasted_cycles);
  printf("wasted_invocations: %lu\n", wasted_invocations);
  printf("wasted_cycles_per_invocation: %.2f\n", wasted_invocations == 0 ? 0.00 : (double)wasted_cycles/(double)wasted_invocations);
  printf("wasted_seconds: %.2f\n", wasted_invocations == 0 ? 0.00 : (double)wasted_cycles/((double)cpu_frequency_khz * 1000)/(double)num_workers);
  printf("==================================================\n");
}
#endif

#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
volatile pthread_spinlock_t promotion_lock;
static volatile uint64_t promotion_generic_cycles = 0;
static volatile uint64_t promotion_generic_counts = 0;
static volatile uint64_t promotion_optimized_cycles = 0;
static volatile uint64_t promotion_optimized_counts = 0;

void collect_heartbeat_promotion_time_init() {
  pthread_spin_init(&promotion_lock, 0);
}

void collect_heartbeat_promotion_time_print() {
  printf("======== Heartbeat Promotion Time Summary ========\n");
  printf("cpu_frequency_khz: %lu\n", cpu_frequency_khz);
  printf("num_workers: %zu\n", num_workers);

  printf("\n");

  printf("promotion_generic_cycles: %lu\n", promotion_generic_cycles);
  printf("promotion_generic_counts: %lu\n", promotion_generic_counts);
  printf("promotion_generic_cycles_per_count: %.2f\n", promotion_generic_counts == 0 ? 0.00 : (double)promotion_generic_cycles/(double)promotion_generic_counts);
  printf("promotion_generic_seconds: %.2f\n", promotion_generic_counts == 0 ? 0.00 : (double)promotion_generic_cycles/((double)cpu_frequency_khz * 1000)/(double)num_workers);

  printf("\n");

  printf("promotion_optimized_cycles: %lu\n", promotion_optimized_cycles);
  printf("promotion_optimized_counts: %lu\n", promotion_optimized_counts);
  printf("promotion_optimized_cycles_per_count: %.2f\n", promotion_optimized_counts == 0 ? 0.00 : (double)promotion_optimized_cycles/(double)promotion_optimized_counts);
  printf("promotion_optimized_seconds: %.2f\n", promotion_optimized_counts == 0 ? 0.00 : (double)promotion_optimized_cycles/((double)cpu_frequency_khz * 1000)/(double)num_workers);
  printf("==================================================\n");
}
#endif
#endif

#define CACHELINE     8
#define LIVE_IN_ENV   0
#define LIVE_OUT_ENV  1
#define START_ITER    0
#define MAX_ITER      1

/*
 * Benchmark-specific variable indicating the level of nested loop
 * This variable should be defined inside heartbeat_*.hpp
 */
extern uint64_t numLevels;

/*
 * Recursively function to determine the splitting level given the
 * global iteration state
 * 1. if no level decided, return numLevels
 * 2. else return the correct splitting level
 */
int64_t determine_splitting_level(int64_t level) {
  return numLevels;
}

template<typename... uint64_ts>
int64_t determine_splitting_level(
  int64_t level,
  uint64_t startIter,
  uint64_t maxIter,
  uint64_ts... restIters
) {
  if ((maxIter - startIter) >= SMALLEST_GRANULARITY + 1) {
    return level;
  } else {
    return determine_splitting_level(level + 1, restIters...);
  }
}

#if defined(HEARTBEAT_BRANCHES)

#elif defined(HEARTBEAT_VERSIONING)

/*
 * The benchmark-specific function to determine the leftover task to use
 * giving the splittingLevel and receivingLevel
 * This function should be defined inside heartbeat_versioning.hpp
 */
uint64_t getLeftoverTaskIndex(uint64_t splittingLevel, uint64_t myLevel);

/*
 * The benchmark-specific function to determine the leaf task to use
 * giving the receivingLevel
 * This function should be defined inside heartbeat_versioning.hpp
 */
uint64_t getLeafTaskIndex(uint64_t myLevel);

#if defined(LOOP_HANDLER_WITHOUT_LIVE_OUT)

#if !defined(LOOP_HANDLER_OPTIMIZED)

/*
 * Generic loop_handler function for the versioning version WITHOUT live-out environment
 * 1. since there's no live-out environment, caller doesn't need the
 *    return value to determine how to do reduction
 */
void loop_handler(
  uint64_t *startIters,
  uint64_t *maxIters,
  uint64_t *constLiveIns,
  uint64_t **liveInEnvs,
  uint64_t myLevel,
  uint64_t startingLevel,
  uint64_t numLevels,
  void (*splittingTasks[])(uint64_t *, uint64_t *, uint64_t *, uint64_t **, uint64_t, uint64_t),
  void (*leftoverTasks[])(uint64_t *, uint64_t *, uint64_t *, uint64_t **, uint64_t, uint64_t),
  void (*leafTasks[])(uint64_t *, uint64_t *, uint64_t *, uint64_t *)
) {
#if defined(DISABLE_HEARTBEAT_PROMOTION)
  return;
#endif

  /*
   * Determine whether to promote since last promotion
   */
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  uint64_t start, end;
  RDTSC(start);
#endif
  auto &p = taskparts::prev.mine();
  auto n = taskparts::cycles::now();
  if ((p + taskparts::kappa_cycles) > n) {
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  RDTSC(end);
  pthread_spin_lock(&lock);
  wasted_cycles += end - start;
  wasted_invocations += 1;
  pthread_spin_unlock(&lock);
#endif
    return;
  }
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  RDTSC(end);
  pthread_spin_lock(&lock);
  useful_cycles += end - start;
  useful_invocations += 1;
  pthread_spin_unlock(&lock);
#endif
  p = n;

  /*
   * Decide the splitting level
   */
  uint64_t splittingLevel = numLevels;
  for (uint64_t level = startingLevel; level <= myLevel; level++) {
    if (maxIters[level * 8] - startIters[level * 8] >= SMALLEST_GRANULARITY + 1) {
      splittingLevel = level;
      break;
    }
  }
  /*
   * No more work to split at any level
   */
  if (splittingLevel == numLevels) {
    return;
  }

  /*
   * Allocate the new liveInEnvs for both tasks
   */
  uint64_t **liveInEnvsFirst =   (uint64_t **)alloca(sizeof(uint64_t) * numLevels * 8);
  uint64_t **liveInEnvsSecond =  (uint64_t **)alloca(sizeof(uint64_t) * numLevels * 8);

  /*
   * Allocate startIters and maxIters for both tasks
   */
  uint64_t *startItersFirst =   (uint64_t *)alloca(sizeof(uint64_t) * numLevels * 8);
  uint64_t *maxItersFirst =     (uint64_t *)alloca(sizeof(uint64_t) * numLevels * 8);
  uint64_t *startItersSecond =  (uint64_t *)alloca(sizeof(uint64_t) * numLevels * 8);
  uint64_t *maxItersSecond =    (uint64_t *)alloca(sizeof(uint64_t) * numLevels * 8);


  /*
   * Determine the splitting point of the remaining iterations
   */
  // TODO: potential overflow problem
  uint64_t med = (startIters[splittingLevel * 8] + 1 + maxIters[splittingLevel * 8]) / 2;

  /*
   * Set startIters and maxIters for both tasks
   */
  startItersFirst[splittingLevel * 8] = startIters[splittingLevel * 8] + 1;
  maxItersFirst[splittingLevel * 8] = med;
  startItersSecond[splittingLevel * 8] = med;
  maxItersSecond[splittingLevel * 8] = maxIters[splittingLevel * 8];

  /*
   * Reconstruct the environment at the splittingLevel for both tasks
   */
  liveInEnvsFirst[splittingLevel * 8] = liveInEnvs[splittingLevel * 8];
  liveInEnvsSecond[splittingLevel * 8] = liveInEnvs[splittingLevel * 8];

  if (splittingLevel == myLevel) { // no leftover task needed
    /*
     * Splitting the rest of work depending on whether splitting the leaf loop
     */
    if (myLevel + 1 != numLevels) {
#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  RDTSC(promotion_end);
  pthread_spin_lock(&promotion_lock);
  promotion_optimized_cycles += promotion_end - promotion_start;
  promotion_optimized_counts += 1;
  pthread_spin_unlock(&promotion_lock);
#endif
      taskparts::tpalrts_promote_via_nativefj([&] {
        (*splittingTasks[myLevel])(startItersFirst, maxItersFirst, constLiveIns, liveInEnvsFirst, myLevel, splittingLevel);
      }, [&] {
        (*splittingTasks[myLevel])(startItersSecond, maxItersSecond, constLiveIns, liveInEnvsSecond, myLevel, splittingLevel);
      }, [] { }, taskparts::bench_scheduler());

    } else {
      /*
       * Determine which optimized leaf task to run
       */
      uint64_t leafTaskIndex = getLeafTaskIndex(myLevel);

#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  RDTSC(promotion_end);
  pthread_spin_lock(&promotion_lock);
  promotion_optimized_cycles += promotion_end - promotion_start;
  promotion_optimized_counts += 1;
  pthread_spin_unlock(&promotion_lock);
#endif
      taskparts::tpalrts_promote_via_nativefj([&] {
        (*leafTasks[leafTaskIndex])(&startItersFirst[myLevel * 8], &maxItersFirst[myLevel * 8], constLiveIns, liveInEnvsFirst[myLevel * 8]);
      }, [&] {
        (*leafTasks[leafTaskIndex])(&startItersSecond[myLevel * 8], &maxItersSecond[myLevel * 8], constLiveIns, liveInEnvsSecond[myLevel * 8]);
      }, [] { }, taskparts::bench_scheduler());
    }

  } else { // build up the leftover task
    /*
     * Allocate the new liveInEnvs for leftover task
     */
    uint64_t **liveInEnvsLeftover = (uint64_t **)alloca(sizeof(uint64_t) * numLevels * 8);

    /*
     * Allocate startIters and maxIters for leftover task
     */
    uint64_t *startItersLeftover =  (uint64_t *)alloca(sizeof(uint64_t) * numLevels * 8);
    uint64_t *maxItersLeftover =    (uint64_t *)alloca(sizeof(uint64_t) * numLevels * 8);

    /*
     * Reconstruct the environment starting from splittingLevel + 1 up to myLevel for leftover task
     */
    for (uint64_t level = splittingLevel + 1; level <= myLevel; level++) {
      uint64_t index = level * 8;
      liveInEnvsLeftover[index] = liveInEnvs[index];

      startItersLeftover[index]  = startIters[index] + 1;
      maxItersLeftover[index] = maxIters[index];

      /*
       * Set maxIters for the tail task
       */
      maxIters[index] = startIters[index] + 1; 
    }

    /*
     * Determine which leftover task to run
     */
    uint64_t leftoverTaskIndex = getLeftoverTaskIndex(splittingLevel, myLevel);

#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  RDTSC(promotion_end);
  pthread_spin_lock(&promotion_lock);
  promotion_optimized_cycles += promotion_end - promotion_start;
  promotion_optimized_counts += 1;
  pthread_spin_unlock(&promotion_lock);
#endif
    taskparts::tpalrts_promote_via_nativefj([&] {
      (*leftoverTasks[leftoverTaskIndex])(startItersLeftover, maxItersLeftover, constLiveIns, liveInEnvsLeftover, myLevel, splittingLevel + 1);
    }, [&] {
      taskparts::tpalrts_promote_via_nativefj([&] {
        (*splittingTasks[splittingLevel])(startItersFirst, maxItersFirst, constLiveIns, liveInEnvsFirst, splittingLevel, splittingLevel);
      }, [&] {
        (*splittingTasks[splittingLevel])(startItersSecond, maxItersSecond, constLiveIns, liveInEnvsSecond, splittingLevel, splittingLevel);
      }, [&] { }, taskparts::bench_scheduler());
    }, [&] { }, taskparts::bench_scheduler());
  }

  /*
   * Reset maxIters for the tail task
   */
  maxIters[splittingLevel * 8] = startIters[splittingLevel * 8] + 1;

  return;
}

#endif

void loop_handler_optimized(
  uint64_t *startIter,
  uint64_t *maxIter,
  uint64_t *constLiveIns,
  uint64_t *liveInEnv,
  void (*leafTask)(uint64_t *, uint64_t *, uint64_t *, uint64_t *)
) {
#if defined(DISABLE_HEARTBEAT_PROMOTION)
  return;
#endif

  /*
   * Determine whether to promote since last promotion
   */
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  uint64_t start, end;
  RDTSC(start);
#endif
  auto &p = taskparts::prev.mine();
  auto n = taskparts::cycles::now();
  if ((p + taskparts::kappa_cycles) > n) {
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  RDTSC(end);
  pthread_spin_lock(&lock);
  wasted_cycles += end - start;
  wasted_invocations += 1;
  pthread_spin_unlock(&lock);
#endif
    return;
  }
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  RDTSC(end);
  pthread_spin_lock(&lock);
  useful_cycles += end - start;
  useful_invocations += 1;
  pthread_spin_unlock(&lock);
#endif
  p = n;

  /*
   * Determine if there's more work for splitting
   */
  if ((maxIter[0 * 8] - startIter[0 * 8]) <= SMALLEST_GRANULARITY) {
    return;
  }

  /*
   * Allocate startIter and maxIter for both tasks
   */
  uint64_t startIterFirst[1 * 8];
  uint64_t maxIterFirst[1 * 8];
  uint64_t startIterSecond[1 * 8];
  uint64_t maxIterSecond[1 * 8];

  uint64_t med = (startIter[0 * 8] + maxIter[0 * 8] + 1) / 2;
  startIterFirst[0 * 8] = startIter[0 * 8] + 1;
  maxIterFirst[0 * 8] = med;
  startIterSecond[0 * 8] = med;
  maxIterSecond[0 * 8] = maxIter[0 * 8];

#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  RDTSC(promotion_end);
  pthread_spin_lock(&promotion_lock);
  promotion_optimized_cycles += promotion_end - promotion_start;
  promotion_optimized_counts += 1;
  pthread_spin_unlock(&promotion_lock);
#endif
  taskparts::tpalrts_promote_via_nativefj([&] {
    (*leafTask)(startIterFirst, maxIterFirst, constLiveIns, liveInEnv);
  }, [&] {
    (*leafTask)(startIterSecond, maxIterSecond, constLiveIns, liveInEnv);
  }, [] { }, taskparts::bench_scheduler());

  /*
   * Set maxIters for the tail task
   */
  maxIter[0 * 8] = startIter[0 * 8] + 1;

  return;
}

#elif defined(LOOP_HANDLER_WITH_LIVE_OUT)

#if !defined(LOOP_HANDLER_OPTIMIZED)

/*
 * Generic loop_handler function for the versioning version WITH live-out environment
 * 1. caller side should prepare the live-out environment for kids,
 *    loop_handler does simply copy the existing live-out environments
 * 2. if no heartbeat promotion happens, return 0
 *    if heartbeat promotion happens, return the number of levels that maxIter needs to be reset
 */
template<typename... uint64_ts>
int64_t loop_handler(
  uint64_t *cxts,
  uint64_t receivingLevel,
  int64_t (*sliceTasks[])(uint64_t *, uint64_t, uint64_t, uint64_t, ...),
  int64_t (*leftoverTasks[])(uint64_t *, uint64_t, uint64_t *),
  int64_t (*leafTasks[])(uint64_t *, uint64_t, uint64_t, uint64_t),
  uint64_ts... iters
) {
#if defined(DISABLE_HEARTBEAT_PROMOTION)
  return 0;
#endif

  /*
   * Determine whether to promote since last promotion
   */
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  uint64_t start, end;
  RDTSC(start);
#endif
  auto &p = taskparts::prev.mine();
  auto n = taskparts::cycles::now();
  if ((p + taskparts::kappa_cycles) > n) {
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  RDTSC(end);
  pthread_spin_lock(&lock);
  wasted_cycles += end - start;
  wasted_invocations += 1;
  pthread_spin_unlock(&lock);
#endif
    return 0;
  }
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  RDTSC(end);
  pthread_spin_lock(&lock);
  useful_cycles += end - start;
  useful_invocations += 1;
  pthread_spin_unlock(&lock);
#endif
  p = n;

  /*
   * Decide the splitting level
   */
  uint64_t splittingLevel = (uint64_t)determine_splitting_level(0, iters...);

  /*
   * No more work to split at any level
   */
  if (splittingLevel == numLevels) {
    return 0;
  }

#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  uint64_t promotion_start, promotion_end;
  RDTSC(promotion_start);
#endif
  /*
   * Snapshot global iteration state
   */
  uint64_t *itersArr = (uint64_t *)alloca(sizeof(uint64_t) * numLevels * 2);
  uint64_t index = 0;
  for (const uint64_t iter : {iters...}) {
    itersArr[index++] = iter;
  }

  /*
   * Allocate cxts for both tasks
   */
  uint64_t *cxtsFirst   = (uint64_t *)alloca(sizeof(uint64_t) * numLevels * CACHELINE);
  uint64_t *cxtsSecond  = (uint64_t *)alloca(sizeof(uint64_t) * numLevels * CACHELINE);

  /*
   * Determine the splitting point of the remaining iterations
   */
  uint64_t med = (itersArr[splittingLevel * 2 + START_ITER] + 1 + itersArr[splittingLevel * 2 + MAX_ITER]) / 2;

  /*
   * Reconstruct the context at the splittingLevel for both tasks
   */
  cxtsFirst[splittingLevel * CACHELINE + LIVE_IN_ENV]   = cxts[splittingLevel * CACHELINE + LIVE_IN_ENV];
  cxtsFirst[splittingLevel * CACHELINE + LIVE_OUT_ENV]  = cxts[splittingLevel * CACHELINE + LIVE_OUT_ENV];
  cxtsSecond[splittingLevel * CACHELINE + LIVE_IN_ENV]  = cxts[splittingLevel * CACHELINE + LIVE_IN_ENV];
  cxtsSecond[splittingLevel * CACHELINE + LIVE_OUT_ENV] = cxts[splittingLevel * CACHELINE + LIVE_OUT_ENV];

  if (splittingLevel == receivingLevel) { // no leftover task needed
    /*
     * Split the rest of work depending on whether splitting the leaf loop
     */
    if (receivingLevel + 1 != numLevels) {
#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  RDTSC(promotion_end);
  pthread_spin_lock(&promotion_lock);
  promotion_generic_cycles += promotion_end - promotion_start;
  promotion_generic_counts += 1;
  pthread_spin_unlock(&promotion_lock);
#endif
      taskparts::tpalrts_promote_via_nativefj([&] {
        (*sliceTasks[receivingLevel])(cxtsFirst, 0, itersArr[receivingLevel * 2 + START_ITER] + 1, med);
      }, [&] {
        (*sliceTasks[receivingLevel])(cxtsSecond, 1, med, itersArr[receivingLevel * 2 + MAX_ITER]);
      }, [] { }, taskparts::bench_scheduler());
    } else {
      /*
       * Determine which optimized leaf task to run
       */
      uint64_t leafTaskIndex = getLeafTaskIndex(receivingLevel);

#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  RDTSC(promotion_end);
  pthread_spin_lock(&promotion_lock);
  promotion_generic_cycles += promotion_end - promotion_start;
  promotion_generic_counts += 1;
  pthread_spin_unlock(&promotion_lock);
#endif
      taskparts::tpalrts_promote_via_nativefj([&] {
        (*leafTasks[leafTaskIndex])(&cxtsFirst[receivingLevel * CACHELINE], 0, itersArr[receivingLevel * 2 + START_ITER] + 1, med);
      }, [&] {
        (*leafTasks[leafTaskIndex])(&cxtsSecond[receivingLevel * CACHELINE], 1, med, itersArr[receivingLevel * 2 + MAX_ITER]);
      }, [] { }, taskparts::bench_scheduler());
    }

  } else { // build up the leftover task
    /*
     * Allocate context for leftover task
     */
    uint64_t *cxtsLeftover = (uint64_t *)alloca(sizeof(uint64_t) * numLevels * CACHELINE);

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
    uint64_t leftoverTaskIndex = getLeftoverTaskIndex(splittingLevel, receivingLevel);

#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  RDTSC(promotion_end);
  pthread_spin_lock(&promotion_lock);
  promotion_generic_cycles += promotion_end - promotion_start;
  promotion_generic_counts += 1;
  pthread_spin_unlock(&promotion_lock);
#endif
    taskparts::tpalrts_promote_via_nativefj([&] {
      (*leftoverTasks[leftoverTaskIndex])(cxtsLeftover, 0, itersArr);
    }, [&] {
      taskparts::tpalrts_promote_via_nativefj([&] {
        (*sliceTasks[splittingLevel])(cxtsFirst, 0, itersArr[splittingLevel * 2 + START_ITER] + 1, med);
      }, [&] {
        (*sliceTasks[splittingLevel])(cxtsSecond, 1, med, itersArr[splittingLevel * 2 + MAX_ITER]);
      }, [&] { }, taskparts::bench_scheduler());
    }, [&] { }, taskparts::bench_scheduler());
  }

  /*
   * Return the number of levels that maxIter needs to be reset
   */
  return receivingLevel - splittingLevel + 1;
}

#endif

int64_t loop_handler_optimized(
  uint64_t *cxt,
  uint64_t startIter,
  uint64_t maxIter,
  int64_t (*leafTask)(uint64_t *, uint64_t, uint64_t, uint64_t)
) {
#if defined(DISABLE_HEARTBEAT_PROMOTION)
  return 0;
#endif

  /*
   * Determine whether to promote since last promotion
   */
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  uint64_t start, end;
  RDTSC(start);
#endif
  auto &p = taskparts::prev.mine();
  auto n = taskparts::cycles::now();
  if ((p + taskparts::kappa_cycles) > n) {
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  RDTSC(end);
  pthread_spin_lock(&lock);
  wasted_cycles += end - start;
  wasted_invocations += 1;
  pthread_spin_unlock(&lock);
#endif
    return 0;
  }
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  RDTSC(end);
  pthread_spin_lock(&lock);
  useful_cycles += end - start;
  useful_invocations += 1;
  pthread_spin_unlock(&lock);
#endif
  p = n;

  /*
   * Determine if there's more work for splitting
   */
  if ((maxIter - startIter) <= SMALLEST_GRANULARITY) {
    return 0;
  }

#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  uint64_t promotion_start, promotion_end;
  RDTSC(promotion_start);
#endif
  /*
   * Allocate and construct cxts for both tasks
   */
  uint64_t cxtFirst[1 * CACHELINE];
  uint64_t cxtSecond[1 * CACHELINE];

  uint64_t med  = (startIter + 1 + maxIter) / 2;

  cxtFirst[LIVE_IN_ENV]   = cxt[LIVE_IN_ENV];
  cxtFirst[LIVE_OUT_ENV]  = cxt[LIVE_OUT_ENV];
  cxtSecond[LIVE_IN_ENV]  = cxt[LIVE_IN_ENV];
  cxtSecond[LIVE_OUT_ENV] = cxt[LIVE_OUT_ENV];

#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  RDTSC(promotion_end);
  pthread_spin_lock(&promotion_lock);
  promotion_optimized_cycles += promotion_end - promotion_start;
  promotion_optimized_counts += 1;
  pthread_spin_unlock(&promotion_lock);
#endif
  taskparts::tpalrts_promote_via_nativefj([&] {
    (*leafTask)(cxtFirst, 0, startIter + 1, med);
  }, [&] {
    (*leafTask)(cxtSecond, 1, med, maxIter);
  }, [] { }, taskparts::bench_scheduler());

  return 1;
}

#else

  #error "Need to specific whether or not loop_handler handles live-out, e.g., LOOP_HANDLER_WITHOUT_LIVE_OUT, LOOP_HANDLER_WITH_LIVE_OUT"

#endif

#endif
