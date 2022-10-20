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
  startIterationsFirstHalf[returnLevel]++;
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
      taskparts::tpalrts_promote_via_nativefj([&] {
        (*f[returnLevel])(startIterationsFirstHalf, maxIterationsFirstHalf, (void **)liveInEnvironmentsFirstHalf, returnLevel, nullptr);
      }, [&] {
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