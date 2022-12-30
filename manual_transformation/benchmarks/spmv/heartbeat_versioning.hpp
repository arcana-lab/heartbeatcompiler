#pragma once

#include "loop_handler.hpp"

#define CACHELINE     8
#define LIVE_IN_ENV   0
#define LIVE_OUT_ENV  1
#define START_ITER    0
#define MAX_ITER      1

#define NUM_LEVELS  2
#define LEVEL_ZERO  0
#define LEVEL_ONE   1

/*
 * Benchmark-specific variable indicating the level of nested loop
 * needs to be defined outside the namespace
 */
uint64_t numLevels;

/*
 * User defined function to determine the index of the leftover task
 * needs to be defined outside the namespace
 */
uint64_t getLeftoverTaskIndex(uint64_t splittingLevel, uint64_t myLevel) {
  return 0;
}

/*
 * User defined function to determine the index of the leaf task
 * needs to be defined outside the namespace
 */
uint64_t getLeafTaskIndex(uint64_t myLevel) {
  return 0;
}

namespace spmv {

void spmv_heartbeat_versioning(double *, uint64_t *, uint64_t *, double *, double *, uint64_t);
void HEARTBEAT_loop0(double *, uint64_t *, uint64_t *, double *, double *, uint64_t);
void HEARTBEAT_loop1(double *, uint64_t *, uint64_t *, double *, uint64_t, double &);

int64_t HEARTBEAT_loop0_slice(uint64_t *, uint64_t, uint64_t, uint64_t);
int64_t HEARTBEAT_loop1_slice(uint64_t *, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
__inline__ int64_t HEARTBEAT_loop0_slice_variadic(uint64_t *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter, ...) {
  return HEARTBEAT_loop0_slice(cxts, myIndex, startIter, maxIter);
}
__inline__ int64_t HEARTBEAT_loop1_slice_variadic(uint64_t *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter, ...) {
  return HEARTBEAT_loop1_slice(cxts, myIndex, startIter, maxIter, 0, 0);
}
typedef int64_t(*sliceTaskPointer)(uint64_t *, uint64_t, uint64_t, uint64_t, ...);
sliceTaskPointer sliceTasks[2] = {
  &HEARTBEAT_loop0_slice_variadic,
  &HEARTBEAT_loop1_slice_variadic
};

int64_t HEARTBEAT_loop_1_0_leftover(uint64_t *, uint64_t, uint64_t *);
typedef int64_t(*leftoverTaskPointer)(uint64_t *, uint64_t, uint64_t *);
leftoverTaskPointer leftoverTasks[1] = {
  &HEARTBEAT_loop_1_0_leftover
};

int64_t HEARTBEAT_loop1_optimized(uint64_t *, uint64_t, uint64_t, uint64_t);
typedef int64_t(*leafTaskPointer)(uint64_t *, uint64_t, uint64_t, uint64_t);
leafTaskPointer leafTasks[1] = {
  &HEARTBEAT_loop1_optimized
};

static bool run_heartbeat = true;
static uint64_t *constLiveIns;

// Entry function for the benchmark
void spmv_heartbeat_versioning(double *val, uint64_t *row_ptr, uint64_t *col_ind, double *x, double *y, uint64_t n) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // initialize number of levels for this nested loop
    numLevels = NUM_LEVELS;

    // allocate const live-ins
    constLiveIns = (uint64_t *)alloca(sizeof(uint64_t) * 5);
    constLiveIns[0] = (uint64_t)row_ptr;
    constLiveIns[1] = (uint64_t)y;
    constLiveIns[2] = (uint64_t)val;
    constLiveIns[3] = (uint64_t)col_ind;
    constLiveIns[4] = (uint64_t)x;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS * CACHELINE];

    // invoke loop0 in heartbeat form
    HEARTBEAT_loop0_slice(cxts, 0, 0, n);

    run_heartbeat = true;
  } else {
    HEARTBEAT_loop0(val, row_ptr, col_ind, x, y, n);
  }
}

// Outlined loops
void HEARTBEAT_loop0(double *val, uint64_t *row_ptr, uint64_t *col_ind, double *x, double *y, uint64_t n) {
  for (uint64_t i = 0; i < n; i++) {
    double r = 0.0;
    HEARTBEAT_loop1(val, row_ptr, col_ind, x, i, r);
    y[i] = r;
  }

  return;
}

void HEARTBEAT_loop1(double *val, uint64_t *row_ptr, uint64_t *col_ind, double *x, uint64_t i, double &r) {
  for (uint64_t k = row_ptr[i]; k < row_ptr[i + 1]; k++) {
    r += val[k] * x[col_ind[k]];
  }

  return;
}

// Cloned loops
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

      rc = HEARTBEAT_loop1_slice(cxts, 0, row_ptr[low], row_ptr[low + 1], low, maxIter);
      if (rc > 0) {
        high = low + 1;
      }

      y[low] = r + redArrLiveOut0Loop1[0 * CACHELINE];
    }
    // exit the chunk execution when either
    // 1. heartbeat promotion happens at a higher nested level and in the process of returnning
    // 2. all iterations are finished
    if (rc > 0 || low == maxIter) {
      break;
    }

    rc = loop_handler(cxts, LEVEL_ZERO, sliceTasks, leftoverTasks, nullptr, low - 1, maxIter);
    if (rc > 0) {
      break;
    }
    startIter = low;
  }
#else
  for (; startIter < maxIter; startIter++) {
    double r = 0.0;

    rc = HEARTBEAT_loop1_slice(cxts, 0, row_ptr[startIter], row_ptr[startIter + 1], startIter, maxIter);
    if (rc > 0) {
      maxIter = startIter + 1;
    }

    y[startIter] = r + redArrLiveOut0Loop1[0 * CACHELINE];

    if (rc > 0) {
      continue;
    } else {
      rc = loop_handler(cxts, LEVEL_ZERO, sliceTasks, leftoverTasks, nullptr, startIter, maxIter);
      if (rc > 0) {
        maxIter = startIter + 1;
      }
    }
  }
#endif

  return rc - 1;
}

int64_t HEARTBEAT_loop1_slice(uint64_t *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter, uint64_t startIter0, uint64_t maxIter0) {
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

  // initialize reduction array for kids
  redArrLiveOut0Kids[0 * CACHELINE] = 0.0;
  redArrLiveOut0Kids[1 * CACHELINE] = 0.0;

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

    rc = loop_handler(cxts, LEVEL_ONE, sliceTasks, leftoverTasks, leafTasks, startIter0, maxIter0, low - 1, maxIter);
    if (rc > 0) {
      break;
    }
    startIter = low;
  }
#else
  for (; startIter < maxIter; startIter++) {
    r_private += val[startIter] * x[col_ind[startIter]];
    rc = loop_handler(cxts, LEVEL_ONE, sliceTasks, leftoverTasks, leafTasks, startIter0, maxIter0, startIter, maxIter);
    if (rc > 0) {
      maxIter = startIter + 1;
    }
  }
#endif

  // reduction
  redArrLiveOut0[myIndex * CACHELINE] += r_private + redArrLiveOut0Kids[0 * CACHELINE] + redArrLiveOut0Kids[1 * CACHELINE];

  // reset live-out environment
  cxts[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0;

  return rc - 1;
}

// Leftover loops
int64_t HEARTBEAT_loop_1_0_leftover(uint64_t *cxts, uint64_t myIndex, uint64_t *itersArr) {
  // load startIter and maxIter
  uint64_t startIter = itersArr[LEVEL_ONE * 2 + START_ITER];
  uint64_t maxIter   = itersArr[LEVEL_ONE * 2 + MAX_ITER];

#ifndef LEFTOVER_SPLITTABLE

  // load const live-ins
  double *val = (double *)constLiveIns[2];
  uint64_t *col_ind = (uint64_t *)constLiveIns[3];
  double *x = (double *)constLiveIns[4];

  // initialize my private copy of reduction array
  double *redArrLiveOut0 = (double *)cxts[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV];
  redArrLiveOut0[myIndex * CACHELINE] = 0.0;

  double r_private = 0.0;
  for (; startIter < maxIter; startIter++) {
    r_private += val[startIter] * x[col_ind[startIter]];
  }

  // reduction
  redArrLiveOut0[myIndex * CACHELINE] += r_private;

  return 0;

#else

  return HEARTBEAT_loop1_slice(cxts, myIndex, startIter, maxIter, 0, 0);

#endif

}

// Cloned optimized leaf loops
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

  // initialize reduction array for kids
  redArrLiveOut0Kids[0 * CACHELINE] = 0.0;
  redArrLiveOut0Kids[1 * CACHELINE] = 0.0;

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

    rc = loop_handler_optimized(cxt, low - 1, maxIter, &HEARTBEAT_loop1_optimized);
    if (rc > 0) {
      break;
    }
    startIter = low;
  }
#else
  for (; startIter < maxIter; startIter++) {
    r_private += val[startIter] * x[col_ind[startIter]];
    rc = loop_handler_optimized(cxt, startIter, maxIter, &HEARTBEAT_loop1_optimized);
    if (rc > 0) {
      maxIter = startIter + 1;
    }
  }
#endif

  // reduction
  redArrLiveOut0[myIndex * CACHELINE] += r_private + redArrLiveOut0Kids[0 * CACHELINE] + redArrLiveOut0Kids[1 * CACHELINE];

  // reset live-out environment
  cxt[LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0;

  return rc - 1;
}

}
