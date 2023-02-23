#include "loop_handler.hpp"
#include <alloca.h>
#include <taskparts/benchmark.hpp>
#include <functional>

extern "C" {

#ifndef SMALLEST_GRANULARITY
  #error "Macro SMALLEST_GRANULARITY undefined"
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
uint64_t numLevels = 2;

void loop_dispatcher(std::function<void()> const& lambda) {
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    lambda();
  });

  return;
}

/*
 * The benchmark-specific function to determine the leftover task to use
 * giving the splittingLevel and receivingLevel
 * This function should be defined inside heartbeat_versioning.hpp
 */
uint64_t getLeftoverTaskIndex(uint64_t splittingLevel, uint64_t myLevel) {
  return 0;
}

#include "code_loop_slice_declaration.hpp"

/*
 * Generic loop_handler function for the versioning version
 * 1. if no heartbeat promotion happens, return 0
 *    if heartbeat promotion happens, 
 *      return the number of levels that maxIter needs to be reset
 * 2. caller side should prepare the live-out environment (if any) for kids,
 *    loop_handler does simply copy the existing live-out environments
 */
int64_t loop_handler(
  void *cxts,
  uint64_t receivingLevel,
  #include "code_loop_handler_signature.hpp"
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
  uint64_t splittingLevel = numLevels;
  #include "code_splitting_level_determination.hpp"

  /*
   * No more work to split at any level
   */
  if (splittingLevel == numLevels) {
    return 0;
  }

  /*
   * Snapshot global iteration state
   */
  uint64_t *itersArr = (uint64_t *)alloca(sizeof(uint64_t) * numLevels * 2);
  uint64_t index = 0;
  #include "code_iteration_state_snapshot.hpp"

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
  #include "code_slice_context_construction.hpp"

  if (splittingLevel == receivingLevel) { // no leftover task needed
    #include "code_slice_task_invocation.hpp"

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
      #include "code_leftover_context_construction.hpp"

      /*
       * Rreset global iters for leftover task
       */
      itersArr[level * 2 + START_ITER] += 1;
    }

    /*
     * Determine which leftover task to run
     */
    uint64_t leftoverTaskIndex = getLeftoverTaskIndex(splittingLevel, receivingLevel);
    #include "code_leftover_task_invocation.hpp"
  }

  /*
   * Return the number of levels that maxIter needs to be reset
   */
  return receivingLevel - splittingLevel + 1;
}

}
