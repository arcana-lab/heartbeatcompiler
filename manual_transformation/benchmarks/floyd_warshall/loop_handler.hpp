#pragma once

#include <taskparts/benchmark.hpp>

void loop_dispatcher(
  void (*f)(int *, int),
  int *dist,
  int vertices
) {
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    (*f)(dist, vertices);
  });
}

#if defined(HEARTBEAT_BRANCHES)

int loop_handler(
  uint64_t *startIterations,
  uint64_t *maxIterations,
  void **liveInEnvironments,
  uint64_t myLevel,
  uint64_t &returnLevel,
  uint64_t (*f[])(uint64_t *, uint64_t *, void **, uint64_t)/*, taskparts::future *fut */
) {
  // urgently getting back to the splitting level
  if (returnLevel < myLevel) {
    return 1;
  }

  static std::atomic_bool *me = taskparts::hardware_alarm_polling_interrupt::my_heartbeat_flag();

  // heartbeat doesn't happen
  if (!(*me)) { 
    return 0;
  }

  // heartbeat happens
  (*me) = false;

  // decide the splitting level
  for (uint64_t level = 0; level <= myLevel; level++) {
    if (maxIterations[level] - startIterations[level] >= 3) {
      returnLevel = level;
      break;
    }
  }
  // no more work to split at any level
  if (returnLevel > myLevel) {
    return 0;
  }

  // allocate the new liveInEnvironments for both tasks
  uint64_t *liveInEnvironmentsFirstHalf[2];
  uint64_t *liveInEnvironmentsSecondHalf[2];
  for (uint64_t level = 0; level <= returnLevel; level++) {
    liveInEnvironmentsFirstHalf[level] = ((uint64_t **)liveInEnvironments)[level];
    liveInEnvironmentsSecondHalf[level] = ((uint64_t **)liveInEnvironments)[level];
  }

  // determine the splitting point of the remaining iterations
  uint64_t med = (startIterations[returnLevel] + 1 + maxIterations[returnLevel]) / 2;

  // allocate startIterations and maxIterations for both tasks
  uint64_t startIterationsFirstHalf[2] =  { startIterations[0], startIterations[1]  };
  uint64_t maxIterationsFirstHalf[2] =    { maxIterations[0],   maxIterations[1]    };
  uint64_t startIterationsSecondHalf[2] = { startIterations[0], startIterations[1]  };
  uint64_t maxIterationsSecondHalf[2] =   { maxIterations[0],   maxIterations[1]    };

  // set startIterations and maxIterations for both tasks
  if (returnLevel != myLevel) {
    startIterationsFirstHalf[returnLevel]++;
  }
  maxIterationsFirstHalf[returnLevel] = med;
  startIterationsSecondHalf[returnLevel] = med;

#if defined(DEBUG_LOOP_HANDLER)
  printf("Loop_handler: Start\n");
  printf("Loop_handler:   Promotion\n");
  printf("Loop_handler:   Receiving Level = %lu\n", myLevel);
  printf("Loop_handler:   Splitting Level = %lu\n", returnLevel);
  printf("Loop_handler:     startIteration = %lu, maxIterations = %lu\n", startIterations[returnLevel], maxIterations[returnLevel]);
  printf("Loop_handler:     med = %lu\n", med);
  printf("Loop_handler:     task1: startIteration = %lu, maxIterations = %lu\n", startIterationsFirstHalf[returnLevel], maxIterationsFirstHalf[returnLevel]);
  printf("Loop_handler:     task2: startIteration = %lu, maxIterations = %lu\n", startIterationsSecondHalf[returnLevel], maxIterationsSecondHalf[returnLevel]);
#endif

  // reset maxIterations for the leftover task
  maxIterations[returnLevel] = startIterations[returnLevel] + 1;

  /*
    assert(fut == nullptr && "Assigning to a future pointer which already bounded\n");
    fut = new lazy_future([=] {
      taskparts::tpalrts_promote_via_nativefj([=] {
        (*f[returnLevel])(startIterationsFirstHalf, maxIterationsFirstHalf, (void **)liveInEnvironmentsFirstHalf, returnLevel, nullptr);
      }, [=] {
        (*f[returnLevel])(startIterationsSecondHalf, maxIterationsSecondHalf, (void **)liveInEnvironmentsSecondHalf, returnLevel, nullptr);
      }, [] { }, taskparts::bench_scheduler());
    }
  */

  taskparts::tpalrts_promote_via_nativefj([&] {
    (*f[returnLevel])(startIterationsFirstHalf, maxIterationsFirstHalf, (void **)liveInEnvironmentsFirstHalf, returnLevel/*, nullptr */);
  }, [&] {
    (*f[returnLevel])(startIterationsSecondHalf, maxIterationsSecondHalf, (void **)liveInEnvironmentsSecondHalf, returnLevel/*, nullptr */);
  }, [] { }, taskparts::bench_scheduler());

  return 1;
}

#elif defined(HEARTBEAT_VERSIONING)

int loop_handler(
  uint64_t *startIterations,
  uint64_t *maxIterations,
  void **liveInEnvironments,
  uint64_t myLevel,
  void (*splittingTasks[])(uint64_t *, uint64_t *, void **, uint64_t),
  void (*leftoverTasks[])(uint64_t *, uint64_t *, void **, uint64_t)
) {
  static std::atomic_bool *me = taskparts::hardware_alarm_polling_interrupt::my_heartbeat_flag();

  // heartbeat doesn't happen
  if (!(*me)) { 
    return 0;
  }

  // heartbeat happens
  (*me) = false;

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
  uint64_t *liveInEnvironmentsFirstHalf[2 * 8];
  uint64_t *liveInEnvironmentsSecondHalf[2 * 8];
  for (uint64_t level = 0; level <= splittingLevel; level++) {
    liveInEnvironmentsFirstHalf[level * 8] = ((uint64_t **)liveInEnvironments)[level * 8];
    liveInEnvironmentsSecondHalf[level * 8] = ((uint64_t **)liveInEnvironments)[level * 8];
  }

  // determine the splitting point of the remaining iterations
  uint64_t med = (startIterations[splittingLevel * 8] + 1 + maxIterations[splittingLevel * 8]) / 2;

  // allocate startIterations and maxIterations for both tasks
  uint64_t startIterationsFirstHalf[2 * 8] =  { startIterations[0 * 8], 0,0,0,0,0,0,0, startIterations[1 * 8], 0,0,0,0,0,0,0 };
  uint64_t maxIterationsFirstHalf[2 * 8] =    { maxIterations[0 * 8],   0,0,0,0,0,0,0, maxIterations[1 * 8]  , 0,0,0,0,0,0,0 };
  uint64_t startIterationsSecondHalf[2 * 8] = { startIterations[0 * 8], 0,0,0,0,0,0,0, startIterations[1 * 8], 0,0,0,0,0,0,0 };
  uint64_t maxIterationsSecondHalf[2 * 8] =   { maxIterations[0 * 8],   0,0,0,0,0,0,0, maxIterations[1 * 8]  , 0,0,0,0,0,0,0 };

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

    taskparts::tpalrts_promote_via_nativefj([&] {
      (*splittingTasks[myLevel])(startIterationsFirstHalf, maxIterationsFirstHalf, (void **)liveInEnvironmentsFirstHalf, myLevel);
    }, [&] {
      (*splittingTasks[myLevel])(startIterationsSecondHalf, maxIterationsSecondHalf, (void **)liveInEnvironmentsSecondHalf, myLevel);
    }, [] { }, taskparts::bench_scheduler());
  } else { // splittingLevel is higher than myLevel, build up the leftover task
    // allocate the new liveInEnvironments for leftover task
    uint64_t *liveInEnvironmentsLeftover[3 * 8];
    for (uint64_t level = 0; level <= myLevel; level++) {
      liveInEnvironmentsLeftover[level * 8] = ((uint64_t **)liveInEnvironments)[level * 8];
    }

    // allocate startIterations and maxIterations for leftover task
    uint64_t startIterationsLeftover[2 * 8] =  { startIterations[0 * 8], 0,0,0,0,0,0,0, startIterations[1 * 8], 0,0,0,0,0,0,0 };
    uint64_t maxIterationsLeftover[2 * 8] =    { maxIterations[0 * 8],   0,0,0,0,0,0,0, maxIterations[1 * 8]  , 0,0,0,0,0,0,0 };

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
    if (myLevel == 1 && (splittingLevel == 0 || splittingLevel == 1)) {
      leftoverTaskIndex = 0;
    }

    taskparts::tpalrts_promote_via_nativefj([&] {
      (*leftoverTasks[leftoverTaskIndex])(startIterationsLeftover, maxIterationsLeftover, (void **)liveInEnvironmentsLeftover, myLevel);
    }, [&] {
      taskparts::tpalrts_promote_via_nativefj([&] {
        (*splittingTasks[splittingLevel])(startIterationsFirstHalf, maxIterationsFirstHalf, (void **)liveInEnvironmentsFirstHalf, splittingLevel);
      }, [&] {
        (*splittingTasks[splittingLevel])(startIterationsSecondHalf, maxIterationsSecondHalf, (void **)liveInEnvironmentsSecondHalf, splittingLevel);
      }, [&] { }, taskparts::bench_scheduler());
    }, [&] { }, taskparts::bench_scheduler());
  }

  return 1;
}

#endif
