#include <atomic>
#include <taskparts/benchmark.hpp>
#include "Heartbeats.hpp"
#include <pthread.h>

extern "C" {

  static pthread_spinlock_t lock;
  __attribute__((constructor))
  void lock_constructor () {
    if ( pthread_spin_init ( &lock, 0 ) != 0 ) {
        exit ( 1 );
    }
  }

  /*
   * Return code for loop_handler
   * -1: nothing happens, return and keep executing the remaining iterations
   * otherwise: heartbeat happens, indicating the returnLevel
   */
  int64_t loop_handler (
      uint64_t *startIterations, 
      uint64_t *maxIterations, 
      void **liveInEnvironments,
      uint64_t (*f[])(uint64_t *, uint64_t *, void **, uint64_t),
      uint64_t myLevel,
      uint64_t *returnLevel) {
    static std::atomic_bool * me = taskparts::hardware_alarm_polling_interrupt::my_heartbeat_flag();

    // if we need to return back, heartbeat splits already
    // or we reach the start of the next iteration of the leftover task at the return level
    if (*returnLevel < myLevel) {
      return 1;
    }

    /*
     * Check if an heartbeat happened.
     */
    if (!(*me)) {
      return 0;
    }

    // reach here with the following guaranteed facts
    // 1. we are not in the process of jumping up loops
    // 2. heartbeat event happened

    /*
     * A heartbeat happened.
     *
     * Step 1: reset the thread-specific heartbeat
     */
    (*me) = false;

    // Deciding the splitting level
    uint64_t splittingLevel = -1;
    for (uint64_t level = 0; level <= myLevel; level++) {
      if (maxIterations[level] - startIterations[level] >= 3) {
        splittingLevel = level;
        break;
      }
    }
    if (splittingLevel == -1) {
      return 0;
    }

    // set returnLevel variable
    *returnLevel = splittingLevel;

    uint64_t med;
    if (splittingLevel == myLevel) {
      med = (startIterations[splittingLevel] + maxIterations[splittingLevel]) / 2;
    } else {
      med = (startIterations[splittingLevel] + 1 + maxIterations[splittingLevel]) / 2;
    }

    // need to allocate the startIterations and endIterations array for both tasks
    // this allocation should be statically decided at compile time
    uint64_t startIterationsFirstHalf[2] = { startIterations[0], startIterations[1] };
    uint64_t maxIterationsFirstHalf[2] = { maxIterations[0], maxIterations[1] };
    uint64_t startIterationsSecondHalf[2] = { startIterations[0], startIterations[1] };
    uint64_t maxIterationsSecondHalf[2] = { maxIterations[0], maxIterations[1] };

    if (splittingLevel != myLevel) {
      startIterationsFirstHalf[splittingLevel]++;
      // need to reset the original maxIteration of the leftover task
    }
    maxIterationsFirstHalf[splittingLevel] = med;
    startIterationsSecondHalf[splittingLevel] = med;

    printf("Loop_handler: Start\n");
    printf("Loop_handler:   Promotion\n");
    printf("Loop_handler:   Receiving Level = %lu\n", myLevel);
    printf("Loop_handler:   Splitting Level = %lu\n", splittingLevel);
    printf("Loop_handler:     startIteration = %lu, maxIterations = %lu\n", startIterations[splittingLevel], maxIterations[splittingLevel]);
    printf("Loop_handler:     med = %lu\n", med);
    printf("Loop_handler:     task1: startIteration = %lu, maxIterations = %lu\n", startIterationsFirstHalf[splittingLevel], maxIterationsFirstHalf[splittingLevel]);
    printf("Loop_handler:     task2: startIteration = %lu, maxIterations = %lu\n", startIterationsSecondHalf[splittingLevel], maxIterationsSecondHalf[splittingLevel]);

    maxIterations[splittingLevel] = startIterations[splittingLevel] + 1;

    // allocate the new liveInEnvironments for both tasks
    uint64_t *liveInEnvironmentsFirstHalf[2];
    uint64_t *liveInEnvironmentsSecondHalf[2];
    for (uint64_t level = 0; level <= splittingLevel; level++) {
      liveInEnvironmentsFirstHalf[level] = ((uint64_t **)liveInEnvironments)[level];
      liveInEnvironmentsSecondHalf[level] = ((uint64_t **)liveInEnvironments)[level];
    }

    taskparts::tpalrts_promote_via_nativefj([&] {
      (*f[splittingLevel])(startIterationsFirstHalf, maxIterationsFirstHalf, (void **)liveInEnvironmentsFirstHalf, splittingLevel);
    }, [&] {
      (*f[splittingLevel])(startIterationsSecondHalf, maxIterationsSecondHalf, (void **)liveInEnvironmentsSecondHalf, splittingLevel);
    }, [] { }, taskparts::bench_scheduler());

    return 1;
  }

  void loop_dispatcher (
      uint64_t *startIterations,
      uint64_t *maxIterations,
      void **liveInEnvironments,
      uint64_t (*f)(uint64_t *, uint64_t *, void **, uint64_t)) {
    taskparts::benchmark_nativeforkjoin([&] (auto sched) {
        (*f)(startIterations, maxIterations, liveInEnvironments, 0);
    });

    return;
  }

}
