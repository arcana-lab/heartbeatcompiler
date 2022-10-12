#include <atomic>
#include <taskparts/benchmark.hpp>
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
   * 1: heartbeat happens at least of the current level
   * 0: nothing happens
   */
  int64_t loop_handler (
      uint64_t *startIterations, 
      uint64_t *maxIterations, 
      void **liveInEnvironments,
      void (*splittingTasks[])(uint64_t *, uint64_t *, void **, uint64_t),
      void (*leftoverTasks[])(uint64_t *, uint64_t *, void **, uint64_t),
      uint64_t myLevel) {
    static std::atomic_bool * me = taskparts::hardware_alarm_polling_interrupt::my_heartbeat_flag();

    /*
     * Check if an heartbeat happened.
     */
    if (!(*me)) {
      return 0;
    }

    /*
     * A heartbeat happened.
     *
     * Step 1: reset the thread-specific heartbeat
     */
    (*me) = false;

    // Decide the splitting level
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

    // Determine the mid point for the splitted task
    // No matter whether the splittingLevel is the myLevel or not, the tail task will always execute one iteration
    // of the current loop, doing that ensures that we don't need to check the return code of loop_handler if 
    // there's no live-out environment
    // the trade-off is that we make the induction variable load and store through shared memory
    // make the current iteration on the critical span of the execution
    // more, need to discuss what roll forwarding is actually implemented
    uint64_t med = (startIterations[splittingLevel] + 1 + maxIterations[splittingLevel]) / 2;

    // Allocate the startIterations and endIterations array for both tasks
    // first splitted task
    uint64_t startIterationsFirstHalf[3] = { startIterations[0], startIterations[1], startIterations[2] };
    uint64_t maxIterationsFirstHalf[3] = { maxIterations[0], maxIterations[1], maxIterations[2] };
    // second splitted task
    uint64_t startIterationsSecondHalf[3] = { startIterations[0], startIterations[1], startIterations[2] };
    uint64_t maxIterationsSecondHalf[3] = { maxIterations[0], maxIterations[1], maxIterations[2] };

    startIterationsFirstHalf[splittingLevel] += 1;  // starts at next iteration of the splitting loop
    maxIterationsFirstHalf[splittingLevel] = med;
    startIterationsSecondHalf[splittingLevel] = med;

    // allocate the new liveInEnvironments for both tasks
    uint64_t *liveInEnvironmentsFirstHalf[3];
    uint64_t *liveInEnvironmentsSecondHalf[3];
    for (uint64_t level = 0; level <= splittingLevel; level++) {
      liveInEnvironmentsFirstHalf[level] = ((uint64_t **)liveInEnvironments)[level];
      liveInEnvironmentsSecondHalf[level] = ((uint64_t **)liveInEnvironments)[level];
    }

    // When the splittingLevel == myLevel, this eases things and won't be any leftover task
    // but only tail task
    if (splittingLevel == myLevel) {

      printf("Loop_handler: Start\n");
      printf("Loop_handler:   Promotion\n");
      printf("Loop_handler:   Receiving Level = %lu\n", myLevel);
      printf("Loop_handler:   Splitting Level = %lu\n", splittingLevel);
      printf("Loop_handler:   Current iteration state:\n");
      for (uint64_t level = 0; level <= myLevel; level++) {
        printf("Loop_handler:     level = %lu, startIteration = %lu, maxIteration = %lu\n", level, startIterations[level], maxIterations[level]);
      }
      printf("Loop_handler:   med = %lu\n", med);
      printf("Loop_handler:     task1:      startIteration = %lu, maxIterations = %lu\n", startIterationsFirstHalf[myLevel], maxIterationsFirstHalf[myLevel]);
      printf("Loop_handler:     task2:      startIteration = %lu, maxIterations = %lu\n", startIterationsSecondHalf[myLevel], maxIterationsSecondHalf[myLevel]);
      printf("Loop_handler:     tail:\n");
      // Set the current iteration to be running for the tail task
      maxIterations[myLevel] = startIterations[myLevel] + 1;
      for (uint64_t level = splittingLevel; level <= myLevel; level++) {
        printf("Loop_handler:       level = %lu, startIteration = %lu, maxIterations = %lu\n", level, startIterations[level], maxIterations[level]);
      }

      taskparts::tpalrts_promote_via_nativefj([&] {
        (*splittingTasks[myLevel])(startIterationsFirstHalf, maxIterationsFirstHalf, (void **)liveInEnvironmentsFirstHalf, myLevel);
      }, [&] {
        (*splittingTasks[myLevel])(startIterationsSecondHalf, maxIterationsSecondHalf, (void **)liveInEnvironmentsSecondHalf, myLevel);
      }, [] { }, taskparts::bench_scheduler());

    } else {  // splittingLevel is higher than myLevel, build up the leftover task

      // live-in environment for leftover task is a bit different as versus the splitting task
      uint64_t *liveInEnvironmentsLeftover[3];
      for (uint64_t level = 0; level <= myLevel; level++) {
        liveInEnvironmentsLeftover[level] = ((uint64_t **)liveInEnvironments)[level];
      }

      // leftover task
      uint64_t startIterationsLeftover[3] = { startIterations[0], startIterations[1], startIterations[2] };
      uint64_t maxIterationsLeftover[3] = { maxIterations[0], maxIterations[1], maxIterations[2] };

      // Set the iterations to be running for the leftover task
      for (uint64_t level = splittingLevel + 1; level <= myLevel; level++) {
        startIterationsLeftover[level] += 1;
      }

      printf("Loop_handler: Start\n");
      printf("Loop_handler:   Promotion\n");
      printf("Loop_handler:   Receiving Level = %lu\n", myLevel);
      printf("Loop_handler:   Splitting Level = %lu\n", splittingLevel);
      printf("Loop_handler:   Current iteration state:\n");
      for (uint64_t level = 0; level <= myLevel; level++) {
        printf("Loop_handler:     level = %lu, startIteration = %lu, maxIteration = %lu\n", level, startIterations[level], maxIterations[level]);
      }
      printf("Loop_handler:   med = %lu\n", med);
      printf("Loop_handler:     task1:      startIteration = %lu, maxIterations = %lu\n", startIterationsFirstHalf[splittingLevel], maxIterationsFirstHalf[splittingLevel]);
      printf("Loop_handler:     task2:      startIteration = %lu, maxIterations = %lu\n", startIterationsSecondHalf[splittingLevel], maxIterationsSecondHalf[splittingLevel]);
      printf("Loop_handler:     leftover:\n");
      for (uint64_t level = splittingLevel + 1; level <= myLevel; level++) {
        printf("Loop_handler:       level = %lu, startIteration = %lu, maxIterations = %lu\n", level, startIterationsLeftover[level], maxIterationsLeftover[level]);
      }
      printf("Loop_handler:     tail:\n");
      // Set the current iteration to be running for the tail task
      for (uint64_t level = splittingLevel; level <= myLevel; level++) {
        maxIterations[level] = startIterations[level] + 1;
        printf("Loop_handler:       level = %lu, startIteration = %lu, maxIterations = %lu\n", level, startIterations[level], maxIterations[level]);
      }

      // determine which leftover task to run
      uint64_t leftoverTaskIndex = 0;
      if (splittingLevel == 0 && myLevel == 1) {
        leftoverTaskIndex = 0;
      } else if (splittingLevel == 1 && myLevel == 2) {
        leftoverTaskIndex = 1;
      } else if (splittingLevel == 0 && myLevel == 2) {
        leftoverTaskIndex = 2;
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

  void loop_dispatcher (
      uint64_t *startIterations,
      uint64_t *maxIterations,
      void **liveInEnvironments,
      void (*f)(uint64_t *, uint64_t *, void **, uint64_t)) {
    taskparts::benchmark_nativeforkjoin([&] (auto sched) {
        (*f)(startIterations, maxIterations, liveInEnvironments, 0);
    });

    return;
  }

}
