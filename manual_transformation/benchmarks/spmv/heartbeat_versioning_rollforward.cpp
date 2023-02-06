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
  uint64_t *row_ptr = (uint64_t *)constLiveIns[0];
  double *y = (double *)constLiveIns[1];

  // allocate reduction array (as live-out environment) for loop1
  double redArrLiveOut0Loop1[1 * CACHELINE];
  cxts[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0Loop1;

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t low, high;
  for (; ;) {
    low = startIter;
    high = std::min(maxIter, startIter + CHUNKSIZE_0);

    for (; low < high; low++) {
      double r = 0.0;

      rc = HEARTBEAT_loop1_slice(cxts, 0, low, maxIter, row_ptr[low], row_ptr[low + 1]);
      if (rc > 0) {
        high = low + 1;
      }

      y[low] = r + redArrLiveOut0Loop1[0 * CACHELINE];
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
    startIter = low;
  }
#else
  for (; startIter < maxIter; startIter++) {
    double r = 0.0;

    rc = HEARTBEAT_loop1_slice(cxts, 0, startIter, maxIter, row_ptr[startIter], row_ptr[startIter + 1]);

    y[startIter] = r + redArrLiveOut0Loop1[0 * CACHELINE];

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

  return rc - 1;
}

int64_t HEARTBEAT_loop1_slice(uint64_t *cxts, uint64_t myIndex, uint64_t startIter0, uint64_t maxIter0, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  double *val = (double *)constLiveIns[2];
  uint64_t *col_ind = (uint64_t *)constLiveIns[3];
  double *x = (double *)constLiveIns[4];

  // initialize my private copy of reduction array
  double *redArrLiveOut0 = (double *)cxts[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV];
  redArrLiveOut0[myIndex * CACHELINE] = 0.0;

  // allocate reduction array (as live-out environment) for kids
  double redArrLiveOut0Kids[2 * CACHELINE];
  cxts[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0Kids;

  int64_t rc = 0;
  double r_private = 0.0;
#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t low, high;
  for (; ;) {
    low = startIter;
    high = std::min(maxIter, startIter + CHUNKSIZE_1);

    for (; low < high; low++) {
      r_private += val[low] * x[col_ind[low]];
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
    startIter = low;
  }
#else
  for (; startIter < maxIter; startIter++) {
    r_private += val[startIter] * x[col_ind[startIter]];
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
    redArrLiveOut0[myIndex * CACHELINE] += r_private + redArrLiveOut0Kids[0 * CACHELINE] + redArrLiveOut0Kids[1 * CACHELINE];
  } else if (rc > 1) { // splittingLevel < myLevel
    redArrLiveOut0[myIndex * CACHELINE] += r_private + redArrLiveOut0Kids[0 * CACHELINE];
  } else { // no heartbeat promotion happens or splittingLevel > myLevel
    redArrLiveOut0[myIndex * CACHELINE] += r_private;
  }

  // reset live-out environment
  cxts[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0;

  return rc - 1;
}

// Transformed optimized loop slice
int64_t HEARTBEAT_loop1_optimized(uint64_t *cxt, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  double *val = (double *)constLiveIns[2];
  uint64_t *col_ind = (uint64_t *)constLiveIns[3];
  double *x = (double *)constLiveIns[4];

  // initialize my private copy of reduction array
  double *redArrLiveOut0 = (double *)cxt[LIVE_OUT_ENV];
  redArrLiveOut0[myIndex * CACHELINE] = 0.0;

  // allocate reduction array (as live-out environment) for kids
  double redArrLiveOut0Kids[2 * CACHELINE];
  cxt[LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0Kids;

  int64_t rc = 0;
  double r_private = 0.0;
#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t low, high;
  for (; ;) {
    low = startIter;
    high = std::min(maxIter, startIter + CHUNKSIZE_1);

    for (; low < high; low++) {
      r_private += val[low] * x[col_ind[low]];
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
    startIter = low;
  }
#else
  for (; startIter < maxIter; startIter++) {
    r_private += val[startIter] * x[col_ind[startIter]];
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
    redArrLiveOut0[myIndex * CACHELINE] += r_private;
  } else {  // splittingLevel == myLevel
    assert(rc == 1);
    redArrLiveOut0[myIndex * CACHELINE] += r_private + redArrLiveOut0Kids[0 * CACHELINE] + redArrLiveOut0Kids[1 * CACHELINE];
  }

  // reset live-out environment
  cxt[LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0;

  return rc - 1;
}
