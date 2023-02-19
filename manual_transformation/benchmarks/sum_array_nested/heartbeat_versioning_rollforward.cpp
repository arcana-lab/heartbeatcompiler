#include "rollforward_handler.hpp"
#include <cassert>

#define CACHELINE     8
#define LIVE_IN_ENV   0
#define LIVE_OUT_ENV  1

#define LEVEL_ZERO  0
#define LEVEL_ONE   1

#include "code_loop_slice_declaration.hpp"

extern int64_t HEARTBEAT_loop_1_0_leftover(uint64_t *, uint64_t, uint64_t *);
typedef int64_t(*leftoverTaskPointer)(uint64_t *, uint64_t, uint64_t *);
leftoverTaskPointer leftoverTasks[1] = {
  &HEARTBEAT_loop_1_0_leftover
};

int64_t HEARTBEAT_loop1_optimized(uint64_t *, uint64_t, uint64_t, uint64_t);
typedef int64_t(*leafTaskPointer)(uint64_t *, uint64_t, uint64_t, uint64_t);
leafTaskPointer leafTasks[1] = {
  &HEARTBEAT_loop1_optimized
};

extern uint64_t *constLiveIns;

// Transformed loop slice
int64_t HEARTBEAT_loop0_slice(uint64_t *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  uint64_t low2 = (uint64_t)constLiveIns[1];
  uint64_t high2 = (uint64_t)constLiveIns[2];

  // load my private copy of reduction array
  uint64_t *redArrLiveOut0 = (uint64_t *)cxts[LEVEL_ZERO * CACHELINE + LIVE_OUT_ENV];

  // allocate reduction array (as live-out environment) for loop1
  uint64_t redArrLiveOut0Loop1[1 * CACHELINE];
  cxts[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0Loop1;

  // allocate reduction array (as live-out environment) for kids
  uint64_t redArrLiveOut0Kids[2 * CACHELINE];
  cxts[LEVEL_ZERO * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0Kids;

  int64_t rc = 0;
  uint64_t r_private = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_0 != 1
  uint64_t low, high;

  // store into live-in environment for loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&low;

  for (; startIter < maxIter; startIter += CHUNKSIZE_0) {
    low = startIter;
    high = std::min(maxIter, startIter + CHUNKSIZE_0);

    for (; low < high; low++) {
      rc = HEARTBEAT_loop1_slice(cxts, 0, low, maxIter, low2, high2);
      if (rc > 0) {
        high = low + 1;
      }

      r_private += redArrLiveOut0Loop1[0 * 8];
    }
    // exit the chunk execution when either
    // 1. heartbeat promotion happens at a higher nested level and in the process of returning
    // 2. all iterations are finished
    if (rc > 0 || low == maxIter) {
      break;
    }

#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_wrapper(rc, cxts, LEVEL_ZERO, leftoverTasks, nullptr, low - 1, maxIter, 0, 0);
#else
    rc = loop_handler(cxts, LEVEL_ZERO, leftoverTasks, nullptr, low - 1, maxIter, 0, 0);
#endif
    if (rc > 0) {
      break;
    }
  }
#else
  // store into live-in environment for loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&startIter;

  for (; startIter < maxIter; startIter++) {
    rc = HEARTBEAT_loop1_slice(cxts, 0, startIter, maxIter, low2, high2);
    r_private += redArrLiveOut0Loop1[0 * 8];

    if (rc > 0) {
      maxIter = startIter + 1;
      continue;
    }
#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_wrapper(rc, cxts, LEVEL_ZERO, leftoverTasks, nullptr, startIter, maxIter, 0, 0);
#else
    rc = loop_handler(cxts, LEVEL_ZERO, leftoverTasks, nullptr, startIter, maxIter, 0, 0);
#endif
    if (rc > 0) {
      maxIter = startIter + 1;
    }
  }
#endif

  // reduction
  if (rc == 1) { // splittingLevel == myLevel
    redArrLiveOut0[myIndex * CACHELINE] = r_private + redArrLiveOut0Kids[0 * CACHELINE] + redArrLiveOut0Kids[1 * CACHELINE];
  } else if (rc > 1) { // splittingLevel < myLevel
    redArrLiveOut0[myIndex * CACHELINE] = r_private + redArrLiveOut0Kids[0 * CACHELINE];
  } else { // no heartbeat promotion happens or splittingLevel > myLevel
    redArrLiveOut0[myIndex * CACHELINE] = r_private;
  }

  // reset live-out environment
  cxts[LEVEL_ZERO * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0;

  return rc - 1;
}

int64_t HEARTBEAT_loop1_slice(uint64_t *cxts, uint64_t myIndex, uint64_t startIter0, uint64_t maxIter0, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  char **a = (char **)constLiveIns[0];

  // load live-in environment
  uint64_t i = *(uint64_t *)cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV];

  // load my private copy of reduction array
  uint64_t *redArrLiveOut0 = (uint64_t *)cxts[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV];

  // allocate reduction array (as live-out environment) for kids
  uint64_t redArrLiveOut0Kids[2 * CACHELINE];
  cxts[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0Kids;

  int64_t rc = 0;
  uint64_t r_private = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_1 != 1
  for (; startIter < maxIter; startIter += CHUNKSIZE_1) {
    uint64_t low = startIter;
    uint64_t high = std::min(maxIter, startIter + CHUNKSIZE_1);

    for (; low < high; low++) {
      r_private += a[i][low];
    }
    if (low == maxIter) {
      break;
    }

#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_wrapper(rc, cxts, LEVEL_ONE, leftoverTasks, leafTasks, startIter0, maxIter0, low - 1, maxIter);
#else
    rc = loop_handler(cxts, LEVEL_ONE, leftoverTasks, leafTasks, startIter0, maxIter0, low - 1, maxIter);
#endif
    if (rc > 0) {
      break;
    }
  }
#else
  for (; startIter < maxIter; startIter++) {
    r_private += a[i][startIter];
#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_wrapper(rc, cxts, LEVEL_ONE, leftoverTasks, leafTasks, startIter0, maxIter0, startIter, maxIter);
#else
    rc = loop_handler(cxts, LEVEL_ONE, leftoverTasks, leafTasks, startIter0, maxIter0, startIter, maxIter);
#endif
    if (rc > 0) {
      maxIter = startIter + 1;
    }
  }
#endif

  // reduction
  if (rc == 1) { // splittingLevel == myLevel
    redArrLiveOut0[myIndex * CACHELINE] = r_private + redArrLiveOut0Kids[0 * CACHELINE] + redArrLiveOut0Kids[1 * CACHELINE];
  } else if (rc > 1) { // splittingLevel < myLevel
    redArrLiveOut0[myIndex * CACHELINE] = r_private + redArrLiveOut0Kids[0 * CACHELINE];
  } else { // no heartbeat promotion happens or splittingLevel > myLevel
    redArrLiveOut0[myIndex * CACHELINE] = r_private;
  }

  // reset live-out environment
  cxts[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0;

  return rc - 1;
}

// Transformed optimized loop slice
int64_t HEARTBEAT_loop1_optimized(uint64_t *cxt, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  char **a = (char **)constLiveIns[0];

  // load live-in environment
  uint64_t i = *(uint64_t *)cxt[LIVE_IN_ENV];

  // load my private copy of reduction array
  uint64_t *redArrLiveOut0 = (uint64_t *)cxt[LIVE_OUT_ENV];

  // allocate reduction array (as live-out environment) for kids
  uint64_t redArrLiveOut0Kids[2 * CACHELINE];
  cxt[LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0Kids;

  int64_t rc = 0;
  uint64_t r_private = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_1 != 1
  for (; startIter < maxIter; startIter += CHUNKSIZE_1) {
    uint64_t low = startIter;
    uint64_t high = std::min(maxIter, startIter + CHUNKSIZE_1);

    for (; low < high; low++) {
      r_private += a[i][low];
    }
    if (low == maxIter) {
      break;
    }

#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_optimized_wrapper(rc, cxt, low - 1, maxIter, &HEARTBEAT_loop1_optimized);
#else
    rc = loop_handler_optimized(cxt, low - 1, maxIter, &HEARTBEAT_loop1_optimized);
#endif
    if (rc > 0) {
      break;
    }
  }
#else
  for (; startIter < maxIter; startIter++) {
    r_private += a[i][startIter];
#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_optimized_wrapper(rc, cxt, startIter, maxIter, &HEARTBEAT_loop1_optimized);
#else
    rc = loop_handler_optimized(cxt, startIter, maxIter, &HEARTBEAT_loop1_optimized);
#endif
    if (rc > 0) {
      maxIter = startIter + 1;
    }
  }
#endif

  // reduction
  if (rc == 0) { // no heartbeat promotion happens
    redArrLiveOut0[myIndex * CACHELINE] = r_private;
  } else {  // splittingLevel == myLevel
    assert(rc == 1);
    redArrLiveOut0[myIndex * CACHELINE] = r_private + redArrLiveOut0Kids[0 * CACHELINE] + redArrLiveOut0Kids[1 * CACHELINE];
  }

  // reset live-out environment
  cxt[LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0;

  return rc - 1;
}
