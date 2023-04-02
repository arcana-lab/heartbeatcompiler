#include "heartbeat_manual.hpp"
#include "loop_handler.hpp"
#include <cstdint>
#include <alloca.h>

#define NUM_LEVELS_NEST0 2
#define LEVEL_ZERO 0
#define LEVEL_ONE 1
#define CACHELINE 8
#define LIVE_IN_ENV 0
#define LIVE_OUT_ENV 1
#define START_ITER 0
#define MAX_ITER 1

namespace nested_plus_reduce_array {

double HEARTBEAT_nest0_loop0(double **a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2);
double HEARTBEAT_nest0_loop1(double **a, uint64_t lo2, uint64_t hi2, uint64_t i);

int64_t HEARTBEAT_nest0_loop0_slice(void *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter);
inline void HEARTBEAT_nest0_loop0_slice_wrapper(void *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  HEARTBEAT_nest0_loop0_slice(cxts, myIndex, startIter, maxIter);
}
int64_t HEARTBEAT_nest0_loop1_slice(void *cxts, uint64_t myIndex, uint64_t s0, uint64_t m0, uint64_t startIter, uint64_t maxIter);
inline void HEARTBEAT_nest0_loop1_slice_wrapper(void *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  HEARTBEAT_nest0_loop1_slice(cxts, myIndex, 0, 0, startIter, maxIter);
}
typedef void (*sliceTasksWrapperPointer)(void *, uint64_t, uint64_t, uint64_t);
sliceTasksWrapperPointer slice_tasks_nest0[2] = {
  &HEARTBEAT_nest0_loop0_slice_wrapper,
  &HEARTBEAT_nest0_loop1_slice_wrapper
};

void HEARTBEAT_nest0_loop_1_0_leftover(void *cxts, uint64_t myIndex, void *itersArr);
typedef void (*leftoverTasksPointer)(void *, uint64_t, void *);
leftoverTasksPointer leftover_tasks_nest0[1] = {
  &HEARTBEAT_nest0_loop_1_0_leftover
};

uint64_t leftover_selector_nest0(uint64_t receivingLevel, uint64_t splittingLevel) {
  return 0;
}

bool run_heartbeat = true;
uint64_t *constLiveIns_nest0;

// Outlined loops
double HEARTBEAT_nest0_loop0(double **a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2) {
  double r = 0.0;
  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate const live-ins
    constLiveIns_nest0 = (uint64_t *)alloca(sizeof(uint64_t) * 3);
    constLiveIns_nest0[0] = (uint64_t)a;
    constLiveIns_nest0[1] = (uint64_t)lo2;
    constLiveIns_nest0[2] = (uint64_t)hi2;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS_NEST0 * CACHELINE];

    // allocate reduction array (as live-out environment) for loop0
    double redArrLoop0LiveOut0[1 * CACHELINE];
    cxts[LEVEL_ZERO * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLoop0LiveOut0;

    // invoke nest0_loop0 in heartbeat form
    HEARTBEAT_nest0_loop0_slice((void *)cxts, 0, lo1, hi1);

    // reduction
    r += redArrLoop0LiveOut0[0 * CACHELINE];

    run_heartbeat = true;
  } else {
    for (uint64_t i = lo1; i != hi1; i++) {
      r += HEARTBEAT_nest0_loop1(a, lo2, hi2, i);
    }
  }
  return r;
}

double HEARTBEAT_nest0_loop1(double **a, uint64_t lo2, uint64_t hi2, uint64_t i) {
  double r = 0.0;
  for (uint64_t j = lo2; j != hi2; j++) {
    r += a[i][j];
  }
  return r;
}

// Transformed loops
int64_t HEARTBEAT_nest0_loop0_slice(void *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  uint64_t lo2 = (uint64_t)constLiveIns_nest0[1];
  uint64_t hi2 = (uint64_t)constLiveIns_nest0[2];

  // load reduction array for live-outs and create a private copy
  double *redArrLoop0LiveOut0 = (double *)((uint64_t *)cxts)[LEVEL_ZERO * CACHELINE + LIVE_OUT_ENV];
  double live_out_0 = 0.0;

  // allocate reduction array (as live-out environment) for kids of loop0
  double redArrLoop0LiveOut0Kids[2 * CACHELINE];
  ((uint64_t *)cxts)[LEVEL_ZERO * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLoop0LiveOut0Kids;

  // allocate reduction array (as live-out environment) for loop1
  double redArrLoop1LiveOut0[1 * CACHELINE];
  ((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLoop1LiveOut0;

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_0 != 1
  uint64_t low, high;
  // store &live-in as live-in environment for loop1
  ((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&low;

  for (; startIter < maxIter; startIter += CHUNKSIZE_0) {
    low = startIter;
    high = maxIter < startIter + CHUNKSIZE_0 ? maxIter : startIter + CHUNKSIZE_0;
    for (; low < high; low++) {
      rc = HEARTBEAT_nest0_loop1_slice(cxts, 0, low, maxIter, lo2, hi2);
      if (rc > 0) {
        // update the exit condition here because there might
        // be tail work to finish
        high = low + 1;
      }
      live_out_0 += redArrLoop1LiveOut0[0 * CACHELINE];
    }

    // exit the chunk execution when either
    // 1. heartbeat promotion happens at a higher nested level and in the process of returnning
    // 2. all iterations are finished
    if (rc > 0 || low == maxIter) {
      break;
    }

#if !defined(ENABLE_ROLLFORWARD)
    rc = loop_handler_level2(
      cxts, LEVEL_ZERO,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0,
      low - 1, maxIter,
      0, 0
    );
#else
    __rf_handle_level2_wrapper(
      rc, cxts, LEVEL_ZERO,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0,
      low - 1, maxIter,
      0, 0
    );
#endif
    if (rc > 0) {
      break;
    }
  }
#else
  // store &live-in as live-in environment for loop1
  ((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&startIter;

  for (; startIter < maxIter; startIter++) {
    rc = HEARTBEAT_nest0_loop1_slice(cxts, 0, startIter, maxIter, lo2, hi2);
    if (rc > 0) {
      // update the exit condition here because there might
      // be tail work to finish
      maxIter = startIter + 1;
    }
    live_out_0 += redArrLoop1LiveOut0[0 * CACHELINE];

    // check the status of rc because, might not need
    // to call the loop_handler in the process of returnning up
    if (rc > 0) {
      break;
    }

#if !defined(ENABLE_ROLLFORWARD)
    rc = loop_handler_level2(
      cxts, LEVEL_ZERO,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0,
      startIter, maxIter,
      0, 0
    );
#else
    __rf_handle_level2_wrapper(
      rc, cxts, LEVEL_ZERO,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0,
      startIter, maxIter,
      0, 0
    );
#endif
    if (rc > 0) {
      break;
    }
  }
#endif

  // reduction
  if (rc == 1) {        // splittingLevel == myLevel
    redArrLoop0LiveOut0[myIndex * CACHELINE] = live_out_0 + redArrLoop0LiveOut0Kids[0 * CACHELINE] + redArrLoop0LiveOut0Kids[1 * CACHELINE];
  } else if (rc > 1) {  // splittingLevel < myLevel
    redArrLoop0LiveOut0[myIndex * CACHELINE] = live_out_0 + redArrLoop0LiveOut0Kids[0 * CACHELINE];
  } else {              // no heartbeat promotion happens or splittingLevel > myLevel
    redArrLoop0LiveOut0[myIndex * CACHELINE] = live_out_0;
  }

  // reset live-out environment
  ((uint64_t *)cxts)[LEVEL_ZERO * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLoop0LiveOut0;

  return rc - 1;
}

int64_t HEARTBEAT_nest0_loop1_slice(void *cxts, uint64_t myIndex, uint64_t s0, uint64_t m0, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  double **a = (double **)constLiveIns_nest0[0];

  // load live-ins
  int i = *(int *)((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_IN_ENV];

  // load reduction array for live-outs and create a private copy variable
  double *redArrLoop1LiveOut0 = (double *)((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV];
  double live_out_0 = 0.0;

  // allocate reduction array (as live-out environment) for kids of loop1
  double redArrLoop1LiveOut0Kids[2 * CACHELINE];
  ((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLoop1LiveOut0Kids;

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_1 != 1
  for (; startIter < maxIter; startIter += CHUNKSIZE_1) {
    uint64_t low = startIter;
    uint64_t high = maxIter < startIter + CHUNKSIZE_1 ? maxIter : startIter + CHUNKSIZE_1;
    for (; low < high; low++) {
      live_out_0 += a[i][low];
    }

    if (low == maxIter) {
      break;
    }

#if !defined(ENABLE_ROLLFORWARD)
    rc = loop_handler_level2(
      cxts, LEVEL_ONE,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0,
      s0, m0,
      low - 1, maxIter
    );
#else
    __rf_handle_level2_wrapper(
      rc, cxts, LEVEL_ONE,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0,
      s0, m0,
      low - 1, maxIter
    );
#endif
    if (rc > 0) {
      break;
    }
  }
#else
  for(; startIter < maxIter; startIter++) {
    live_out_0 += a[i][startIter];

#if !defined(ENABLE_ROLLFORWARD)
    rc = loop_handler_level2(
      cxts, LEVEL_ONE,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0,
      s0, m0,
      startIter, maxIter
    );
#else
    __rf_handle_level2_wrapper(
      rc, cxts, LEVEL_ONE,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0,
      s0, m0,
      startIter, maxIter
    );
#endif
    if (rc > 0) {
      break;
    }
  }
#endif

  // rc from a leaf loop slice can only be 0 or 1,
  // however, when considering a non-leaf loop need to
  // consider the scenario of returning up
  // reduction
  if (rc == 0) {  // no heartbeat promotion happens
    redArrLoop1LiveOut0[myIndex * CACHELINE] = live_out_0;
  } else {        // splittingLevel == myLevel
    redArrLoop1LiveOut0[myIndex * CACHELINE] = live_out_0 + redArrLoop1LiveOut0Kids[0 * CACHELINE] + redArrLoop1LiveOut0Kids[1 * CACHELINE];
  }

  // reset live-out environment
  ((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLoop1LiveOut0;

  return rc - 1;
}

// Leftover tasks
void HEARTBEAT_nest0_loop_1_0_leftover(void *cxts, uint64_t myIndex, void *itersArr) {
  // load startIter and maxIter
  uint64_t startIter0 = ((uint64_t *)itersArr)[LEVEL_ZERO * 2 + START_ITER];
  uint64_t maxIter0   = ((uint64_t *)itersArr)[LEVEL_ZERO * 2 + MAX_ITER];
  uint64_t startIter1 = ((uint64_t *)itersArr)[LEVEL_ONE * 2 + START_ITER];
  uint64_t maxIter1   = ((uint64_t *)itersArr)[LEVEL_ONE * 2 + MAX_ITER];

  // KNOWN BUG HERE:
  // In the leftover task and the heartbeat arrives, and it promotes outer parallelism
  // then the second part of the slice task will work on the second entry of the reduction array
  // (if the outer loop contains live-out environment)
  // The reason is because the reduction array for kids of the outer loop hasn't been allocate
  // therefore there will be a data race between the new right slice task and the original right slice task
  int64_t rc = 0;
  rc = HEARTBEAT_nest0_loop1_slice(cxts, myIndex, startIter0, maxIter0, startIter1, maxIter1);
  if (rc > 0) {
    return;
  }

  rc = HEARTBEAT_nest0_loop0_slice(cxts, myIndex, startIter0+1, maxIter0);
  if (rc > 0) {
    return;
  }

  return;
}

} // namespace nested_plus_reduce_array
