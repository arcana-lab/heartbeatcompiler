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
  uint64_t splittingLevel = -1;
  for (uint64_t level = 0; level <= myLevel; level++) {
    if (maxIterations[level] - startIterations[level] >= 3) {
      splittingLevel = level;
      break;
    }
  }
  // no more work to split at any level
  if (splittingLevel == -1) {
    return 0;
  }

  // set returnLevel variable
  returnLevel = splittingLevel;

  // determine the splitting point of the remaining iterations
  uint64_t med;
  if (splittingLevel == myLevel) {
    med = (startIterations[splittingLevel] + maxIterations[splittingLevel]) / 2;
  } else {
    med = (startIterations[splittingLevel] + 1 + maxIterations[splittingLevel]) / 2;
  }

  // allocate the new liveInEnvironments for both tasks
  uint64_t *liveInEnvironmentsFirstHalf[2];
  uint64_t *liveInEnvironmentsSecondHalf[2];
  for (uint64_t level = 0; level <= splittingLevel; level++) {
    liveInEnvironmentsFirstHalf[level] = ((uint64_t **)liveInEnvironments)[level];
    liveInEnvironmentsSecondHalf[level] = ((uint64_t **)liveInEnvironments)[level];
  }

  // allocate startIterations and maxIterations for both tasks
  uint64_t startIterationsFirstHalf[2] =  { startIterations[0], startIterations[1]  };
  uint64_t maxIterationsFirstHalf[2] =    { maxIterations[0],   maxIterations[1]    };
  uint64_t startIterationsSecondHalf[2] = { startIterations[0], startIterations[1]  };
  uint64_t maxIterationsSecondHalf[2] =   { maxIterations[0],   maxIterations[1]    };

  if (splittingLevel != myLevel) {
    startIterationsFirstHalf[splittingLevel]++;
  }
  maxIterationsFirstHalf[splittingLevel] = med;
  startIterationsSecondHalf[splittingLevel] = med;

#if defined(DEBUG_LOOP_HANDLER)
  printf("Loop_handler: Start\n");
  printf("Loop_handler:   Promotion\n");
  printf("Loop_handler:   Receiving Level = %lu\n", myLevel);
  printf("Loop_handler:   Splitting Level = %lu\n", splittingLevel);
  printf("Loop_handler:     startIteration = %lu, maxIterations = %lu\n", startIterations[splittingLevel], maxIterations[splittingLevel]);
  printf("Loop_handler:     med = %lu\n", med);
  printf("Loop_handler:     task1: startIteration = %lu, maxIterations = %lu\n", startIterationsFirstHalf[splittingLevel], maxIterationsFirstHalf[splittingLevel]);
  printf("Loop_handler:     task2: startIteration = %lu, maxIterations = %lu\n", startIterationsSecondHalf[splittingLevel], maxIterationsSecondHalf[splittingLevel]);
#endif

  maxIterations[splittingLevel] = startIterations[splittingLevel] + 1;

  /*
    assert(fut == nullptr && "Assigning to a future pointer which already bounded\n");
    fut = new lazy_future([=] {
      taskparts::tpalrts_promote_via_nativefj([&] {
        (*f[splittingLevel])(startIterationsFirstHalf, maxIterationsFirstHalf, (void **)liveInEnvironmentsFirstHalf, splittingLevel, nullptr);
      }, [&] {
        (*f[splittingLevel])(startIterationsSecondHalf, maxIterationsSecondHalf, (void **)liveInEnvironmentsSecondHalf, splittingLevel, nullptr);
      }, [] { }, taskparts::bench_scheduler());
    }
  */

  taskparts::tpalrts_promote_via_nativefj([&] {
    (*f[splittingLevel])(startIterationsFirstHalf, maxIterationsFirstHalf, (void **)liveInEnvironmentsFirstHalf, splittingLevel/*, nullptr */);
  }, [&] {
    (*f[splittingLevel])(startIterationsSecondHalf, maxIterationsSecondHalf, (void **)liveInEnvironmentsSecondHalf, splittingLevel/*, nullptr */);
  }, [] { }, taskparts::bench_scheduler());

  return 1;
}