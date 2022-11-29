#pragma once

#include <taskparts/benchmark.hpp>

#ifndef HIGHEST_NESTED_LEVEL
  #error "Macro HIGHEST_NESTED_LEVEL undefined"
#endif
#ifndef SMALLEST_GRANULARITY
  #error "Macro SMALLEST_GRANULARITY undefined"
#endif

#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
#include <pthread.h>
#include <stdio.h>

#define RDTSC(rdtsc_val_) do { \
  uint32_t rdtsc_hi_, rdtsc_lo_; \
  __asm__ volatile ("rdtsc" : "=a" (rdtsc_lo_), "=d" (rdtsc_hi_)); \
  rdtsc_val_ = (uint64_t)rdtsc_hi_ << 32 | rdtsc_lo_; \
} while (0)

volatile pthread_spinlock_t lock;
auto num_workers = taskparts::perworker::id::get_nb_workers();
static volatile uint64_t useful_cycles = 0;
static volatile uint64_t useful_invocations = 0;
static volatile uint64_t wasted_cycles = 0;
static volatile uint64_t wasted_invocations = 0;

void collect_heartbeat_polling_time_init() {
  pthread_spin_init(&lock, 0);
}

void collect_heartbeat_polling_time_print() {
  printf("cpu frequency: 2.8 GHz\n");
  printf("num_workers: %zu\n", num_workers);

  printf("useful_cycles: %lu\n", useful_cycles);
  printf("useful_invocations: %lu\n", useful_invocations);
  printf("useful_cycles_per_invocation: %.2f\n", useful_invocations == 0 ? 0 : (double)useful_cycles/(double)useful_invocations);
  printf("useful_seconds: \033[0;32m%.2f\033[0m\n", useful_invocations == 0 ? 0.00 : (double)useful_cycles/(double)2800000000/(double)num_workers);

  printf("wasted_cycles: %lu\n", wasted_cycles);
  printf("wasted_invocations: %lu\n", wasted_invocations);
  printf("wasted_cycles_per_invocation: %.2f\n", wasted_invocations == 0 ? 0 : (double)wasted_cycles/(double)wasted_invocations);
  printf("wasted_seconds: \033[0;31m%.2f\033[0m\n", wasted_invocations == 0 ? 0.00 : (double)wasted_cycles/(double)2800000000/(double)num_workers);
}
#endif

#if defined(HEARTBEAT_BRANCHES)

#elif defined(HEARTBEAT_VERSIONING)

/*
 * The benchmark-specific function to determine the leftover task to use
 * giving the splittingLevel and receivingLevel
 * This function should be defined inside heartbeat_versioning.hpp
 */
uint64_t getLeftoverTaskIndex(uint64_t splittingLevel, uint64_t myLevel);

#if defined(LOOP_HANDLER_WITHOUT_LIVE_OUT)

/*
 * Generic loop_handler function for the versioning version WITHOUT live-out environment
 * 1. since there's no live-out environment, caller doesn't need the
 *    return value to determine how to do reduction
 */
void loop_handler(
  uint64_t *startIters,
  uint64_t *maxIters,
  uint64_t **liveInEnvs,
  uint64_t myLevel,
  void (*splittingTasks[])(uint64_t *, uint64_t *, uint64_t **, uint64_t),
  void (*leftoverTasks[])(uint64_t *, uint64_t *, uint64_t **, uint64_t)
) {
#if defined(DISABLE_HEARTBEAT_PROMOTION)
  return;
#endif

  /*
   * Determine whether to promote since last promotion
   */
  auto &p = taskparts::prev.mine();
  auto n = taskparts::cycles::now();
  if ((p + taskparts::kappa_cycles) > n) {
    return;
  }
  p = n;

  /*
   * Decide the splitting level
   */
  uint64_t splittingLevel = HIGHEST_NESTED_LEVEL + 1;
  for (uint64_t level = 0; level <= myLevel; level++) {
    if (maxIters[level * 8] - startIters[level * 8] >= SMALLEST_GRANULARITY + 1) {
      splittingLevel = level;
      break;
    }
  }
  /*
   * No more work to split at any level
   */
  if (splittingLevel > myLevel) {
    return;
  }

  /*
   * Allocate the new liveInEnvs for both tasks
   */
  uint64_t *liveInEnvsFirst[HIGHEST_NESTED_LEVEL * 8];
  uint64_t *liveInEnvsSecond[HIGHEST_NESTED_LEVEL * 8];

  /*
   * Allocate startIters and maxIters for both tasks
   */
  uint64_t startItersFirst[HIGHEST_NESTED_LEVEL * 8];
  uint64_t maxItersFirst[HIGHEST_NESTED_LEVEL * 8];
  uint64_t startItersSecond[HIGHEST_NESTED_LEVEL * 8];
  uint64_t maxItersSecond[HIGHEST_NESTED_LEVEL * 8];

  /*
   * Determine the splitting point of the remaining iterations
   */
  uint64_t med = (startIters[splittingLevel * 8] + 1 + maxIters[splittingLevel * 8]) / 2;

  /*
   * Set startIters and maxIters for both tasks
   */
  startItersFirst[splittingLevel * 8] = startIters[splittingLevel * 8] + 1;
  maxItersFirst[splittingLevel * 8] = med;
  startItersSecond[splittingLevel * 8] = med;
  maxItersSecond[splittingLevel * 8] = maxIters[splittingLevel * 8];

  /*
   * Reset the startIters and maxIters on previous levels for both tasks
   */
  for (uint64_t level = 0; level < splittingLevel; level++) {
    uint64_t index = level * 8;
    startItersFirst[index]  = 0;
    maxItersFirst[index]    = 0;
    startItersSecond[index] = 0;
    maxItersSecond[index]   = 0;
  }

  /*
   * Reconstruct the environment at the splittingLevel for both tasks
   */
  liveInEnvsFirst[splittingLevel * 8] = liveInEnvs[splittingLevel * 8];
  liveInEnvsSecond[splittingLevel * 8] = liveInEnvs[splittingLevel * 8];

  if (splittingLevel == myLevel) { // no leftover task needed
    /*
     * Reset maxIters for the tail task
     */
    maxIters[myLevel * 8] = startIters[myLevel * 8] + 1;

    taskparts::tpalrts_promote_via_nativefj([&] {
      (*splittingTasks[myLevel])(startItersFirst, maxItersFirst, liveInEnvsFirst, myLevel);
    }, [&] {
      (*splittingTasks[myLevel])(startItersSecond, maxItersSecond, liveInEnvsSecond, myLevel);
    }, [] { }, taskparts::bench_scheduler());

  } else { // build up the leftover task
    /*
     * Allocate the new liveInEnvs for leftover task
     */
    uint64_t *liveInEnvsLeftover[HIGHEST_NESTED_LEVEL * 8];

    /*
     * Allocate startIters and maxIters for leftover task
     */
    uint64_t startItersLeftover[HIGHEST_NESTED_LEVEL * 8];
    uint64_t maxItersLeftover[HIGHEST_NESTED_LEVEL * 8];

#ifdef LEFTOVER_SPLITTABLE
    /*
     * Reset the startIters and maxIters on previous levels for leftover task
     * This is necessary if want the leftover task to be further splittable
     */
    for (uint64_t level = 0; level <= splittingLevel; level++) {
      uint64_t index = level * 8;
      startItersLeftover[index] = 0;
      maxItersLeftover[index] = 0;
    }
#endif

    /*
     * Reconstruct the environment up to myLevel for leftover task
     */
    for (uint64_t level = splittingLevel + 1; level <= myLevel; level++) {
      uint64_t index = level * 8;
      liveInEnvsLeftover[index] = liveInEnvs[index];

      startItersLeftover[index]  = startIters[index] + 1;
      maxItersLeftover[index] = maxIters[index];

      maxIters[index] = startIters[index] + 1; 
    }

    /*
     * Reset maxIters for the tail task
     */
    maxIters[splittingLevel * 8] = startIters[splittingLevel * 8] + 1;

    /*
     * Determine which leftover task to run
     */
    uint64_t leftoverTaskIndex = getLeftoverTaskIndex(splittingLevel, myLevel);

    taskparts::tpalrts_promote_via_nativefj([&] {
      (*leftoverTasks[leftoverTaskIndex])(startItersLeftover, maxItersLeftover, liveInEnvsLeftover, myLevel);
    }, [&] {
      taskparts::tpalrts_promote_via_nativefj([&] {
        (*splittingTasks[splittingLevel])(startItersFirst, maxItersFirst, liveInEnvsFirst, splittingLevel);
      }, [&] {
        (*splittingTasks[splittingLevel])(startItersSecond, maxItersSecond, liveInEnvsSecond, splittingLevel);
      }, [&] { }, taskparts::bench_scheduler());
    }, [&] { }, taskparts::bench_scheduler());
  }

  return;
}

#elif defined(LOOP_HANDLER_WITH_LIVE_OUT)

#include "limits.h"

/*
 * Generic loop_handler function for the versioning version WITH live-out environment
 * 1. caller side should prepare the live-out environment for kids,
 *    loop_handler does simply copy the existing live-out environments
 * 2. if no heartbeat promotion happens, return LLONG_MAX
 *    if heartbeat promotion happens, return splittingLevel
 * 3. caller should perform the correct reduction depends on the return value
 */
uint64_t loop_handler(
  uint64_t *startIters,
  uint64_t *maxIters,
  uint64_t **liveInEnvs,
  uint64_t **liveOutEnvs,
  uint64_t myLevel,
  uint64_t myIndex,
  uint64_t (*splittingTasks[])(uint64_t *, uint64_t *, uint64_t **, uint64_t **, uint64_t, uint64_t),
  uint64_t (*leftoverTasks[])(uint64_t *, uint64_t *, uint64_t **, uint64_t **, uint64_t, uint64_t)
) {
#if defined(DISABLE_HEARTBEAT_PROMOTION)
  return LLONG_MAX;
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
    return LLONG_MAX;
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
  uint64_t splittingLevel = HIGHEST_NESTED_LEVEL + 1;
  for (uint64_t level = 0; level <= myLevel; level++) {
    if (maxIters[level * 8] - startIters[level * 8] >= SMALLEST_GRANULARITY + 1) {
      splittingLevel = level;
      break;
    }
  }
  /*
   * No more work to split at any level
   */
  if (splittingLevel > myLevel) {
    return LLONG_MAX;
  }

  /*
   * Allocate the new liveInEnvs and liveOutEnvs for both tasks
   */
  uint64_t *liveInEnvsFirst[HIGHEST_NESTED_LEVEL * 8];
  uint64_t *liveInEnvsSecond[HIGHEST_NESTED_LEVEL * 8];
  uint64_t *liveOutEnvsFirst[HIGHEST_NESTED_LEVEL * 8];
  uint64_t *liveOutEnvsSecond[HIGHEST_NESTED_LEVEL * 8];

  /*
   * Allocate startIters and maxIters for both tasks
   */
  uint64_t startItersFirst[HIGHEST_NESTED_LEVEL * 8];
  uint64_t maxItersFirst[HIGHEST_NESTED_LEVEL * 8];
  uint64_t startItersSecond[HIGHEST_NESTED_LEVEL * 8];
  uint64_t maxItersSecond[HIGHEST_NESTED_LEVEL * 8];

  /*
   * Determine the splitting point of the remaining iterations
   */
  uint64_t med = (startIters[splittingLevel * 8] + 1 + maxIters[splittingLevel * 8]) / 2;

  /*
   * Set startIters and maxIters for both tasks
   */
  startItersFirst[splittingLevel * 8] = startIters[splittingLevel * 8] + 1;
  maxItersFirst[splittingLevel * 8] = med;
  startItersSecond[splittingLevel * 8] = med;
  maxItersSecond[splittingLevel * 8] = maxIters[splittingLevel * 8];

  /*
   * Reset the startIters and maxIters on previous levels for both tasks
   */
  for (uint64_t level = 0; level < splittingLevel; level++) {
    uint64_t index = level * 8;
    startItersFirst[index]  = 0;
    maxItersFirst[index]    = 0;
    startItersSecond[index] = 0;
    maxItersSecond[index]   = 0;
  }

  /*
   * Reconstruct the environment at the splittingLevel for both tasks
   */
  liveInEnvsFirst[splittingLevel * 8] = liveInEnvs[splittingLevel * 8];
  liveInEnvsSecond[splittingLevel * 8] = liveInEnvs[splittingLevel * 8];
  liveOutEnvsFirst[splittingLevel * 8] = liveOutEnvs[splittingLevel * 8];
  liveOutEnvsSecond[splittingLevel * 8] = liveOutEnvs[splittingLevel * 8];

  if (splittingLevel == myLevel) { // no leftover task needed
    /*
     * Reset maxIters for the tail task
     */
    maxIters[myLevel * 8] = startIters[myLevel * 8] + 1;

    taskparts::tpalrts_promote_via_nativefj([&] {
      (*splittingTasks[myLevel])(startItersFirst, maxItersFirst, liveInEnvsFirst, liveOutEnvsFirst, myLevel, 0);
    }, [&] {
      (*splittingTasks[myLevel])(startItersSecond, maxItersSecond, liveInEnvsSecond, liveOutEnvsSecond, myLevel, 1);
    }, [] { }, taskparts::bench_scheduler());

  } else { // build up the leftover task
    /*
     * Allocate the new liveInEnvs and liveOutEnvs for leftover task
     */
    uint64_t *liveInEnvsLeftover[HIGHEST_NESTED_LEVEL * 8];
    uint64_t *liveOutEnvsLeftover[HIGHEST_NESTED_LEVEL * 8];

    /*
     * Allocate startIters and maxIters for leftover task
     */
    uint64_t startItersLeftover[HIGHEST_NESTED_LEVEL * 8];
    uint64_t maxItersLeftover[HIGHEST_NESTED_LEVEL * 8];

#ifdef LEFTOVER_SPLITTABLE
    /*
     * Reset the startIters and maxIters on previous levels for leftover task
     * This is necessary if want the leftover task to be further splittable
     */
    for (uint64_t level = 0; level <= splittingLevel; level++) {
      uint64_t index = level * 8;
      startItersLeftover[index] = 0;
      maxItersLeftover[index] = 0;
    }
#endif

    /*
     * Reconstruct the environment up to myLevel for leftover task
     */
    for (uint64_t level = splittingLevel + 1; level <= myLevel; level++) {
      uint64_t index = level * 8;
      liveInEnvsLeftover[index] = liveInEnvs[index];
      liveOutEnvsLeftover[index] = liveOutEnvs[index];

      startItersLeftover[index]  = startIters[index] + 1;
      maxItersLeftover[index] = maxIters[index];

      maxIters[index] = startIters[index] + 1; 
    }

    /*
     * Reset maxIters for the tail task
     */
    maxIters[splittingLevel * 8] = startIters[splittingLevel * 8] + 1;

    /*
     * Determine which leftover task to run
     */
    uint64_t leftoverTaskIndex = getLeftoverTaskIndex(splittingLevel, myLevel);

    taskparts::tpalrts_promote_via_nativefj([&] {
      (*leftoverTasks[leftoverTaskIndex])(startItersLeftover, maxItersLeftover, liveInEnvsLeftover, liveOutEnvsLeftover, myLevel, 0);
    }, [&] {
      taskparts::tpalrts_promote_via_nativefj([&] {
        (*splittingTasks[splittingLevel])(startItersFirst, maxItersFirst, liveInEnvsFirst, liveOutEnvsFirst, splittingLevel, 0);
      }, [&] {
        (*splittingTasks[splittingLevel])(startItersSecond, maxItersSecond, liveInEnvsSecond, liveOutEnvsSecond, splittingLevel, 1);
      }, [&] { }, taskparts::bench_scheduler());
    }, [&] { }, taskparts::bench_scheduler());
  }

  return splittingLevel;
}

#else

  #error "Need to specific whether or not loop_handler handles live-out, e.g., LOOP_HANDLER_WITHOUT_LIVE_OUT, LOOP_HANDLER_WITH_LIVE_OUT"

#endif

#elif defined (HEARTBEAT_VERSIONING_OPTIMIZED)

/*
 * Optimized loop_handler WITH live-out environment
 * Assumption: caller loop structure has highest nested level of 1
 */
int loop_handler(
  uint64_t *startIter,
  uint64_t *maxIter,
  uint64_t *liveInEnv,
  uint64_t *liveOutEnv,
  void (*splittingTask)(uint64_t *, uint64_t *, uint64_t *, uint64_t *, uint64_t)
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
   * Optimization: splitting level is always 0
   */
  if ((maxIter[0 * 8] - startIter[0 * 8]) <= SMALLEST_GRANULARITY) {
    return 0;
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

  /*
   * Reset maxIters for the tail task
   */
  maxIter[0 * 8] = startIter[0 * 8] + 1;

  taskparts::tpalrts_promote_via_nativefj([&] {
    (*splittingTask)(startIterFirst, maxIterFirst, liveInEnv, liveOutEnv, 0);
  }, [&] {
    (*splittingTask)(startIterSecond, maxIterSecond, liveInEnv, liveOutEnv, 1);
  }, [] { }, taskparts::bench_scheduler());

  return 1;
}

#endif
