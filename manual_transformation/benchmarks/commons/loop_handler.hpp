#pragma once

#include <taskparts/benchmark.hpp>

#ifndef HIGHEST_NESTED_LEVEL
  #error "Macro HIGHEST_NESTED_LEVEL undefined"
#endif
#ifndef SMALLEST_GRANULARITY
  #error "Macro SMALLEST_GRANULARITY undefined"
#endif

#if defined(HEARTBEAT_BRANCHES)

#elif defined(HEARTBEAT_VERSIONING)

/*
 * The benchmark-specific function to determine the leftover task to use
 * giving the splittingLevel and receivingLevel
 * This function should be defined inside heartbeat_versioning.hpp
 */
uint64_t getLeftoverTaskIndex(uint64_t splittingLevel, uint64_t myLevel);

/*
 * Generic loop_handler function for the versioning version without live-out environment
 */
int loop_handler(
  uint64_t *startIters,
  uint64_t *maxIters,
  uint64_t **liveInEnvs,
  uint64_t myLevel,
  void (*splittingTasks[])(uint64_t *, uint64_t *, uint64_t **, uint64_t),
  void (*leftoverTasks[])(uint64_t *, uint64_t *, uint64_t **, uint64_t)
) {
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
    return 0;
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
   * Reconstruct the environment up to the splittingLevel for both tasks
   */
  for (uint64_t level = 0; level <= splittingLevel; level++) {
    uint64_t index = level * 8;
    liveInEnvsFirst[index] = liveInEnvs[index];
    liveInEnvsSecond[index] = liveInEnvs[index];

    startItersFirst[index]  = startIters[index];
    maxItersFirst[index]    = maxIters[index];
    startItersSecond[index] = startIters[index];
    maxItersSecond[index]   = maxIters[index];
  }

  /*
   * Determine the splitting point of the remaining iterations
   */
  uint64_t med = (startIters[splittingLevel * 8] + 1 + maxIters[splittingLevel * 8]) / 2;

  /*
   * Set startIters and maxIters for both tasks
   */
  startItersFirst[splittingLevel * 8]++;
  maxItersFirst[splittingLevel * 8] = med;
  startItersSecond[splittingLevel * 8] = med;

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

    return 1;
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

    /*
     * Reconstruct the environment up to myLevel for leftover task
     */
    for (uint64_t level = 0; level <= myLevel; level++) {
      uint64_t index = level * 8;
      liveInEnvsLeftover[index] = liveInEnvs[index];

      startItersLeftover[index]  = startIters[index];
      maxItersLeftover[index]    = maxIters[index];
    }

    /*
     * Set the startIters for the leftover task
     */
    for (uint64_t level = splittingLevel + 1; level <= myLevel; level++) {
      startItersLeftover[level * 8]++;
      maxIters[level * 8] = startIters[level * 8] + 1;
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

    return 2;
  }

  /*
   * Unreachable
   */
  exit(1);
}

/*
 * Generic loop_handler function for the versioning version with live-out environment
 */
int loop_handler(
  uint64_t *startIters,
  uint64_t *maxIters,
  uint64_t **liveInEnvs,
  uint64_t **liveOutEnvs,
  uint64_t *liveOutEnvKids,
  uint64_t myLevel,
  uint64_t myIndex,
  void (*splittingTasks[])(uint64_t *, uint64_t *, uint64_t **, uint64_t **, uint64_t, uint64_t),
  void (*leftoverTasks[])(uint64_t *, uint64_t *, uint64_t **, uint64_t **, uint64_t, uint64_t)
) {
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
    return 0;
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
   * Reconstruct the environment up to the splittingLevel for both tasks
   */
  for (uint64_t level = 0; level <= splittingLevel; level++) {
    uint64_t index = level * 8;
    liveInEnvsFirst[index] = liveInEnvs[index];
    liveInEnvsSecond[index] = liveInEnvs[index];
    liveOutEnvsFirst[index] = liveOutEnvs[index];
    liveOutEnvsSecond[index] = liveOutEnvs[index];

    startItersFirst[index]  = startIters[index];
    maxItersFirst[index]    = maxIters[index];
    startItersSecond[index] = startIters[index];
    maxItersSecond[index]   = maxIters[index];
  }

  /*
   * Determine the splitting point of the remaining iterations
   */
  uint64_t med = (startIters[splittingLevel * 8] + 1 + maxIters[splittingLevel * 8]) / 2;

  /*
   * Set startIters and maxIters for both tasks
   */
  startItersFirst[splittingLevel * 8]++;
  maxItersFirst[splittingLevel * 8] = med;
  startItersSecond[splittingLevel * 8] = med;

  if (splittingLevel == myLevel) { // no leftover task needed
    /*
     * Reset maxIters for the tail task
     */
    maxIters[myLevel * 8] = startIters[myLevel * 8] + 1;

    /*
     * Replace splitting level's live-out environment with kids' live-out environment
     */
    liveOutEnvsFirst[myLevel * 8] = liveOutEnvKids;
    liveOutEnvsSecond[myLevel * 8] = liveOutEnvKids;

    taskparts::tpalrts_promote_via_nativefj([&] {
      (*splittingTasks[myLevel])(startItersFirst, maxItersFirst, liveInEnvsFirst, liveOutEnvsFirst, myLevel, 0);
    }, [&] {
      (*splittingTasks[myLevel])(startItersSecond, maxItersSecond, liveInEnvsSecond, liveOutEnvsSecond, myLevel, 1);
    }, [] { }, taskparts::bench_scheduler());

    return 1;
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

    /*
     * Reconstruct the environment up to myLevel for leftover task
     */
    for (uint64_t level = 0; level <= myLevel; level++) {
      uint64_t index = level * 8;
      liveInEnvsLeftover[index] = liveInEnvs[index];
      liveOutEnvsLeftover[index] = liveOutEnvs[index];

      startItersLeftover[index]  = startIters[index];
      maxItersLeftover[index]    = maxIters[index];
    }

    /*
     * Leftover task along works on the next level live-out environment
     */
    liveOutEnvsLeftover[myLevel] = liveOutEnvKids;

    /*
     * Set the startIters for the leftover task
     */
    for (uint64_t level = splittingLevel + 1; level <= myLevel; level++) {
      startItersLeftover[level * 8]++;
      maxIters[level * 8] = startIters[level * 8] + 1;
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

    return 2;
  }

  /*
   * Unreachable
   */
  exit(1);
}

#elif defined (HEARTBEAT_VERSIONING_OPTIMIZED)

/*
 * Optimized loop_handler with live-out environment
 */
int loop_handler(
  uint64_t *startIter,
  uint64_t *maxIter,
  uint64_t *liveInEnv,
  uint64_t *liveOutEnvKids,
  void (*splittingTask)(uint64_t *, uint64_t *, uint64_t *, uint64_t *, uint64_t)
) {
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
    (*splittingTask)(startIterFirst, maxIterFirst, liveInEnv, liveOutEnvKids, 0);
  }, [&] {
    (*splittingTask)(startIterSecond, maxIterSecond, liveInEnv, liveOutEnvKids, 1);
  }, [] { }, taskparts::bench_scheduler());

  return 1;
}

#endif
