#pragma once

#include <taskparts/benchmark.hpp>

void loop_dispatcher(
  void (*f)(char *, uint64_t, uint64_t, uint64_t &),
  char *a,
  uint64_t low,
  uint64_t high,
  uint64_t &result
) {
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    (*f)(a, low, high, result);
  });
}

#if defined(HEARTBEAT_BRANCHES)

#elif defined(HEARTBEAT_VERSIONING)

int loop_handler(
  uint64_t *startIterations,
  uint64_t *maxIterations,
  void **liveInEnvironments,
  void **liveOutEnvironments,
  void *liveOutEnvironmentNextLevel,
  uint64_t myLevel,
  uint64_t myIndex,
  void (*splittingTasks[])(uint64_t *, uint64_t *, void **, void **, uint64_t, uint64_t),
  void (*leftoverTasks[])(uint64_t *, uint64_t *, void **, void **, uint64_t, uint64_t)
) {
  // determine whether to promote since last promotion
  auto& p = taskparts::prev.mine();
  auto n = taskparts::cycles::now();
  if ((p + taskparts::kappa_cycles) > n) {
    return 0;
  }
  p = n;

  // decide the splitting level
  uint64_t splittingLevel = 1 + 1;
  for (uint64_t level = 0; level <= myLevel; level++) {
    if (maxIterations[level * 8] - startIterations[level * 8] >= 3) {
      splittingLevel = level;
      break;
    }
  }
  // no more work to split at any level
  if (splittingLevel > myLevel) {
    return 0;
  }

  // allocate the new liveInEnvironments for both tasks
  uint64_t *liveInEnvironmentsFirstHalf[1];
  uint64_t *liveInEnvironmentsSecondHalf[1];
  for (uint64_t level = 0; level <= splittingLevel; level++) {
    liveInEnvironmentsFirstHalf[level] = ((uint64_t **)liveInEnvironments)[level];
    liveInEnvironmentsSecondHalf[level] = ((uint64_t **)liveInEnvironments)[level];
  }

  // allocate the new liveOutEnvironments for both tasks
  uint64_t *liveOutEnvironmentsFirstHalf[1];
  uint64_t *liveOutEnvironmentsSecondHalf[1];
  for (uint64_t level = 0; level <= splittingLevel; level++) {
    liveOutEnvironmentsFirstHalf[level] = ((uint64_t **)liveOutEnvironments)[level];
    liveOutEnvironmentsSecondHalf[level] = ((uint64_t **)liveOutEnvironments)[level];
  }

  // allocate startIterations and maxIterations for both tasks
  uint64_t startIterationsFirstHalf[1 * 8];
  uint64_t maxIterationsFirstHalf[1 * 8];
  uint64_t startIterationsSecondHalf[1 * 8];
  uint64_t maxIterationsSecondHalf[1* 8];
  for (uint64_t i = 0; i < 1; i++) {
    startIterationsFirstHalf[i * 8]  = startIterations[i * 8];
    maxIterationsFirstHalf[i * 8]    = maxIterations[i * 8];
    startIterationsSecondHalf[i * 8] = startIterations[i * 8];
    maxIterationsSecondHalf[i * 8]   = maxIterations[i * 8];
  }

  // determine the splitting point of the remaining iterations
  uint64_t med = (startIterations[splittingLevel * 8] + 1 + maxIterations[splittingLevel * 8]) / 2;

  // set startIterations and maxIterations for both tasks
  startIterationsFirstHalf[splittingLevel * 8]++;
  maxIterationsFirstHalf[splittingLevel * 8] = med;
  startIterationsSecondHalf[splittingLevel * 8] = med;

  // when splittingLevel == myLevel, this eases things and won't be any leftover task
  if (splittingLevel == myLevel) {

#if defined(DEBUG_LOOP_HANDLER)
    printf("Loop_handler: Start\n");
    printf("Loop_handler:   Promotion\n");
    printf("Loop_handler:   Receiving Level = %lu\n", myLevel);
    printf("Loop_handler:   Splitting Level = %lu\n", splittingLevel);
    printf("Loop_handler:   Current iteration state:\n");
    for (uint64_t level = 0; level <= myLevel; level++) {
      printf("Loop_handler:     level = %lu, startIteration = %lu, maxIteration = %lu\n", level, startIterations[level * 8], maxIterations[level * 8]);
    }
    printf("Loop_handler:     med = %lu\n", med);
    printf("Loop_handler:     task1:      startIteration = %lu, maxIterations = %lu\n", startIterationsFirstHalf[myLevel * 8], maxIterationsFirstHalf[myLevel * 8]);
    printf("Loop_handler:     task2:      startIteration = %lu, maxIterations = %lu\n", startIterationsSecondHalf[myLevel * 8], maxIterationsSecondHalf[myLevel * 8]);
    printf("Loop_handler:     tail:\n");
#endif
    // reset maxIterations for the tail task
    maxIterations[myLevel * 8] = startIterations[myLevel * 8] + 1;
#if defined(DEBUG_LOOP_HANDLER)
    printf("Loop_handler:       level = %lu, startIteration = %lu, maxIterations = %lu\n", level, startIterations[level * 8], maxIterations[level * 8]);
#endif

    // replace splitting level's live-out environment with next level live-out environment
    liveOutEnvironmentsFirstHalf[myLevel] = (uint64_t *)liveOutEnvironmentNextLevel;
    liveOutEnvironmentsSecondHalf[myLevel] = (uint64_t *)liveOutEnvironmentNextLevel;

    taskparts::tpalrts_promote_via_nativefj([&] {
      (*splittingTasks[myLevel])(startIterationsFirstHalf, maxIterationsFirstHalf, (void **)liveInEnvironmentsFirstHalf, (void **)liveOutEnvironmentsFirstHalf, myLevel, 0);
    }, [&] {
      (*splittingTasks[myLevel])(startIterationsSecondHalf, maxIterationsSecondHalf, (void **)liveInEnvironmentsSecondHalf, (void **)liveOutEnvironmentsSecondHalf, myLevel, 1);
    }, [] { }, taskparts::bench_scheduler());

    return 1;

  } else { // splittingLevel is higher than myLevel, build up the leftover task

    // allocate the new liveInEnvironments for leftover task
    uint64_t *liveInEnvironmentsLeftover[1];
    for (uint64_t level = 0; level <= myLevel; level++) {
      liveInEnvironmentsLeftover[level] = ((uint64_t **)liveInEnvironments)[level];
    }

    // allocate the new liveOutEnvironments for leftover task
    uint64_t *liveOutEnvironmentsLeftover[1];
    for (uint64_t level = 0; level < myLevel; level++) {
      liveOutEnvironmentsLeftover[level] = ((uint64_t **)liveOutEnvironments)[level];
    }

    // leftover task along works on the next level live-out environment
    liveOutEnvironmentsLeftover[myLevel] = (uint64_t *)liveOutEnvironmentNextLevel;

    // allocate startIterations and maxIterations for leftover task
    uint64_t startIterationsLeftover[1 * 8];
    uint64_t maxIterationsLeftover[1 * 8];
    for (uint64_t i = 0; i < 1; i++) {
      startIterationsLeftover[i * 8]  = startIterations[i * 8];
      maxIterationsLeftover[i * 8]    = maxIterations[i * 8];
    }

    // set the startIterations for the leftover task
    for (uint64_t level = splittingLevel + 1; level <= myLevel; level++) {
      startIterationsLeftover[level * 8] += 1;
    }

#if defined(DEBUG_LOOP_HANDLER)
    printf("Loop_handler: Start\n");
    printf("Loop_handler:   Promotion\n");
    printf("Loop_handler:   Receiving Level = %lu\n", myLevel);
    printf("Loop_handler:   Splitting Level = %lu\n", splittingLevel);
    printf("Loop_handler:   Current iteration state:\n");
    for (uint64_t level = 0; level <= myLevel; level++) {
      printf("Loop_handler:     level = %lu, startIteration = %lu, maxIteration = %lu\n", level, startIterations[level * 8], maxIterations[level * 8]);
    }
    printf("Loop_handler:     med = %lu\n", med);
    printf("Loop_handler:     task1:      startIteration = %lu, maxIterations = %lu\n", startIterationsFirstHalf[splittingLevel * 8], maxIterationsFirstHalf[splittingLevel * 8]);
    printf("Loop_handler:     task2:      startIteration = %lu, maxIterations = %lu\n", startIterationsSecondHalf[splittingLevel * 8], maxIterationsSecondHalf[splittingLevel * 8]);
    printf("Loop_handler:     leftover:\n");
    for (uint64_t level = splittingLevel + 1; level <= myLevel; level++) {
      printf("Loop_handler:       level = %lu, startIteration = %lu, maxIterations = %lu\n", level, startIterationsLeftover[level * 8], maxIterationsLeftover[level * 8]);
    }
    printf("Loop_handler:     tail:\n");
#endif
    // reset maxIterations for the tail task
    for (uint64_t level = splittingLevel; level <= myLevel; level++) {
      maxIterations[level * 8] = startIterations[level * 8] + 1;
#if defined(DEBUG_LOOP_HANDLER)
      printf("Loop_handler:       level = %lu, startIteration = %lu, maxIterations = %lu\n", level, startIterations[level * 8], maxIterations[level * 8]);
#endif
    }

    // determine which leftover task to run
    uint64_t leftoverTaskIndex = 0;
    if (myLevel == 1 && splittingLevel == 0) {
      leftoverTaskIndex = 0;
    }

    taskparts::tpalrts_promote_via_nativefj([&] {
      (*leftoverTasks[leftoverTaskIndex])(startIterationsLeftover, maxIterationsLeftover, (void **)liveInEnvironmentsLeftover, (void **)liveOutEnvironmentsLeftover, myLevel, 0);
    }, [&] {
      taskparts::tpalrts_promote_via_nativefj([&] {
        (*splittingTasks[splittingLevel])(startIterationsFirstHalf, maxIterationsFirstHalf, (void **)liveInEnvironmentsFirstHalf, (void **)liveOutEnvironmentsFirstHalf, splittingLevel, 0);
      }, [&] {
        (*splittingTasks[splittingLevel])(startIterationsSecondHalf, maxIterationsSecondHalf, (void **)liveInEnvironmentsSecondHalf, (void **)liveOutEnvironmentsSecondHalf, splittingLevel, 1);
      }, [&] { }, taskparts::bench_scheduler());
    }, [&] { }, taskparts::bench_scheduler());

    return 2;
  }

  // unreachable
  exit(1);
}

#elif defined(HEARTBEAT_VERSIONING_OPTIMIZED)

int loop_handler(
  uint64_t startIteration,
  uint64_t &maxIteration,
  void *liveInEnvironment,
  void *liveOutEnvironmentNextLevel,
  void (*splittingTask)(uint64_t, uint64_t, void *, void *, uint64_t)
) {
  // determine whether to promote since last promotion
  auto& p = taskparts::prev.mine();
  auto n = taskparts::cycles::now();
  if ((p + taskparts::kappa_cycles) > n) {
    return 0;
  }
  p = n;

  // decide the splitting level
  // optimization: splitting level is always 0
  if ((maxIteration - startIteration) <= 2) {
    return 0;
  }

  // allocate the new liveInEnvironments for both tasks
  // optimization: two splitting tasks use the same live-in environment

  // allocate the new liveOutEnvironments for both tasks
  // optimization: two splitting tasks use the same live-out environment for next level

  // allocate startIterations and maxIterations for both tasks
  // optimization: no need to allocate this
  uint64_t med = (startIteration + maxIteration + 1) / 2;
  uint64_t startIterationFirstHalf = startIteration + 1;
  uint64_t maxIterationFirstHalf = med;
  uint64_t startIterationSecondHalf = med;
  uint64_t maxIterationSecondHalf = maxIteration;

  // reset maxIterations for the tail task
  maxIteration = startIteration + 1;

  // when splittingLevel == myLevel, this eases things and won't be any leftover task
  // optimization: there won't be a second case

  // replace splitting level's live-out environment with next level live-out environment
  // optimization: use next level live-out environment directly
  taskparts::tpalrts_promote_via_nativefj([&] {
    (*splittingTask)(startIterationFirstHalf, maxIterationFirstHalf, liveInEnvironment, liveOutEnvironmentNextLevel, 0);
  }, [&] {
    (*splittingTask)(startIterationSecondHalf, maxIterationSecondHalf, liveInEnvironment, liveOutEnvironmentNextLevel, 1);
  }, [] { }, taskparts::bench_scheduler());

  return 1;
}

#endif