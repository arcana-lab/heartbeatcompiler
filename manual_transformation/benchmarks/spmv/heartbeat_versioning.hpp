#pragma once

#include "loop_handler.hpp"

#define CACHELINE     8
#define START_ITER    0
#define MAX_ITER      1
#define LIVE_IN_ENV   2
#define LIVE_OUT_ENV  3

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
uint64_t HEARTBEAT_loop0_splitting(uint64_t *, uint64_t, uint64_t, uint64_t);
uint64_t HEARTBEAT_loop1_splitting(uint64_t *, uint64_t, uint64_t, uint64_t);
uint64_t HEARTBEAT_loop1_leftover(uint64_t *, uint64_t, uint64_t, uint64_t);
uint64_t HEARTBEAT_loop1_optimized(uint64_t *, uint64_t);

typedef uint64_t(*functionPointer)(uint64_t *, uint64_t, uint64_t, uint64_t);
functionPointer splittingTasks[2] = {
  &HEARTBEAT_loop0_splitting,
  &HEARTBEAT_loop1_splitting
};
functionPointer leftoverTasks[1] = {
  &HEARTBEAT_loop1_leftover
};
typedef uint64_t(*leafFunctionPointer)(uint64_t *, uint64_t);
leafFunctionPointer leafTasks[1] = {
  &HEARTBEAT_loop1_optimized
};

static bool run_heartbeat = true;
static uint64_t *constLiveIns;

// Entry function for the benchmark
void spmv_heartbeat_versioning(double *val, uint64_t *row_ptr, uint64_t *col_ind, double *x, double *y, uint64_t n) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // initialize number of levels for this nested loop
    numLevels = 2;

    // allocate const live-ins
    constLiveIns = (uint64_t *)alloca(sizeof(uint64_t) * 5);
    constLiveIns[0] = (uint64_t)row_ptr;
    constLiveIns[1] = (uint64_t)y;
    constLiveIns[2] = (uint64_t)val;
    constLiveIns[3] = (uint64_t)col_ind;
    constLiveIns[4] = (uint64_t)x;

    // allocate cxts
    uint64_t cxts[2 * CACHELINE];

    // set the start and max iteration for loop0
    cxts[0 * CACHELINE + START_ITER] = 0;
    cxts[0 * CACHELINE + MAX_ITER] = n;

    // invoke loop0 in heartbeat form
    HEARTBEAT_loop0_splitting(cxts, 0, 0, 0);

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
uint64_t HEARTBEAT_loop0_splitting(uint64_t *cxts, uint64_t myLevel, uint64_t myIndex, uint64_t startingLevel) {
  // load const live-ins
  uint64_t *row_ptr = (uint64_t *)constLiveIns[0];
  double *y = (double *)constLiveIns[1];

  // allocate reduction array (as live-out environment) for loop1
  double redArrLiveOut0Loop1[1 * CACHELINE];
  cxts[(myLevel + 1) * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0Loop1;

#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t low, high;
  for (; ;) {
    low = cxts[myLevel * CACHELINE + START_ITER];
    high = std::min(cxts[myLevel * CACHELINE + MAX_ITER], cxts[myLevel * CACHELINE + START_ITER] + CHUNKSIZE_0);
    cxts[myLevel * CACHELINE + START_ITER] = high - 1;

    for (; low < high; low++) {
      double r = 0.0;

      // set the start and max iteration for loop1
      cxts[(myLevel + 1) * CACHELINE + START_ITER] = row_ptr[low];
      cxts[(myLevel + 1) * CACHELINE + MAX_ITER] = row_ptr[low + 1];

      HEARTBEAT_loop1_splitting(cxts, myLevel + 1, 0, startingLevel);

      y[low] = r + redArrLiveOut0Loop1[0 * CACHELINE];
    }

    if (low == cxts[myLevel * CACHELINE + MAX_ITER]) {
      break;
    }
    loop_handler(cxts, myLevel, startingLevel, splittingTasks, leftoverTasks, nullptr);
    if (low == cxts[myLevel * CACHELINE + MAX_ITER]) {
      break;
    }
    cxts[myLevel * CACHELINE + START_ITER] = low;
  }
#else
  for (; cxts[myLevel * CACHELINE + START_ITER] < cxts[myLevel * CACHELINE + MAX_ITER]; cxts[myLevel * CACHELINE + START_ITER]++) {
    double r = 0.0;

    // set the start and max iteration for loop1
    cxts[(myLevel + 1) * CACHELINE + START_ITER] = row_ptr[cxts[myLevel * CACHELINE + START_ITER]];
    cxts[(myLevel + 1) * CACHELINE + MAX_ITER] = row_ptr[cxts[myLevel * CACHELINE + START_ITER] + 1];

    HEARTBEAT_loop1_splitting(cxts, myLevel + 1, 0, startingLevel);

    y[cxts[myLevel * CACHELINE + START_ITER]] = r + redArrLiveOut0Loop1[0 * CACHELINE];
    loop_handler(cxts, myLevel, startingLevel, splittingTasks, leftoverTasks, nullptr);
  }
#endif

  return LLONG_MAX;
}

uint64_t HEARTBEAT_loop1_splitting(uint64_t *cxts, uint64_t myLevel, uint64_t myIndex, uint64_t startingLevel) {
  // load const live-ins
  double *val = (double *)constLiveIns[2];
  uint64_t *col_ind = (uint64_t *)constLiveIns[3];
  double *x = (double *)constLiveIns[4];

  // initialize my private copy of reduction array
  double *redArrLiveOut0 = (double *)cxts[myLevel * CACHELINE + LIVE_OUT_ENV];
  redArrLiveOut0[myIndex * CACHELINE] = 0.0;

  // allocate reduction array (as live-out environment) for kids
  double redArrLiveOut0Kids[2 * CACHELINE];
  cxts[myLevel * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0Kids;

  uint64_t rc = LLONG_MAX;
  double r_private = 0.0;
#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t low, high;
  for (; ;) {
    low = cxts[myLevel * CACHELINE + START_ITER];
    high = std::min(cxts[myLevel * CACHELINE + MAX_ITER], cxts[myLevel * CACHELINE + START_ITER] + CHUNKSIZE_1);

    for (; low < high; low++) {
      r_private += val[low] * x[col_ind[low]];
    }

    if (low == cxts[myLevel * CACHELINE + MAX_ITER]) {
      break;
    }
    cxts[myLevel * CACHELINE + START_ITER] = low;
    rc = loop_handler(cxts, myLevel, startingLevel, splittingTasks, leftoverTasks, leafTasks);
  }
#else
  for (; cxts[myLevel * CACHELINE + START_ITER] < cxts[myLevel * CACHELINE + MAX_ITER]; cxts[myLevel * CACHELINE + START_ITER]++) {
    r_private += val[cxts[myLevel * CACHELINE + START_ITER]] * x[col_ind[cxts[myLevel * CACHELINE + START_ITER]]];
    rc = loop_handler(cxts, myLevel, startingLevel, splittingTasks, leftoverTasks, leafTasks);
  }
#endif

  // reduction
  if (rc == LLONG_MAX) {      // either no heartbeat promotion happens or promotion happens at a higher nested level
    redArrLiveOut0[myIndex * CACHELINE] += r_private;
  } else if (rc == myLevel) { // heartbeat promotion happens at my level
    redArrLiveOut0[myIndex * CACHELINE] += r_private + redArrLiveOut0Kids[0 * CACHELINE] + redArrLiveOut0Kids[1 * CACHELINE];
    rc = LLONG_MAX;
    goto exit;
  } else {                    // heartbeat promotion happens and splitting happens at a lower nested level
    redArrLiveOut0[myIndex * CACHELINE] += r_private + redArrLiveOut0Kids[0 * CACHELINE];
  }

exit:
  // reset live-out environment
  cxts[myLevel * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0;

  return rc;
}

// Leftover loops
uint64_t HEARTBEAT_loop1_leftover(uint64_t *cxts, uint64_t myLevel, uint64_t myIndex, uint64_t startingLevel) {
#ifndef LEFTOVER_SPLITTABLE

  // load const live-ins
  double *val = (double *)constLiveIns[2];
  uint64_t *col_ind = (uint64_t *)constLiveIns[3];
  double *x = (double *)constLiveIns[4];

  // initialize my private copy of reduction array
  double *redArrLiveOut0 = (double *)cxts[myLevel * CACHELINE + LIVE_OUT_ENV];
  redArrLiveOut0[myIndex * CACHELINE] = 0.0;

  double r_private = 0.0;
  for (uint64_t k = cxts[myLevel * CACHELINE + START_ITER]; k < cxts[myLevel * CACHELINE + MAX_ITER]; k++) {
    r_private += val[k] * x[col_ind[k]];
  }

  // reduction
  redArrLiveOut0[myIndex * CACHELINE] += r_private;

  return 0;

#else

  return HEARTBEAT_loop1_splitting(cxts, myLevel, myIndex, startingLevel);

#endif

}

// Cloned optimized leaf loops
uint64_t HEARTBEAT_loop1_optimized(uint64_t *cxt, uint64_t myIndex) {
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

  uint64_t rc = LLONG_MAX;
  double r_private = 0.0;
#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t low, high;
  for (; ;) {
    low = cxt[START_ITER];
    high = std::min(cxt[MAX_ITER], cxt[START_ITER] + CHUNKSIZE_1);

    for (; low < high; low++) {
      r_private += val[low] * x[col_ind[low]];
    }

    if (low == cxt[MAX_ITER]) {
      break;
    }
    cxt[START_ITER] = low;
    rc = loop_handler_optimized(cxt, &HEARTBEAT_loop1_optimized);
  }
#else
  for (; cxt[START_ITER] < cxt[MAX_ITER]; cxt[START_ITER]++) {
    r_private += val[cxt[START_ITER]] * x[col_ind[cxt[START_ITER]]];
    rc = loop_handler_optimized(cxt, &HEARTBEAT_loop1_optimized);
  }
#endif

  // reduction
  if (rc == LLONG_MAX) {      // no heartbeat promotion happens
    redArrLiveOut0[myIndex * CACHELINE] += r_private;
  } else if (rc == 0) {       // heartbeat promotion happens
    redArrLiveOut0[myIndex * CACHELINE] += r_private + redArrLiveOut0Kids[0 * CACHELINE] + redArrLiveOut0Kids[1 * CACHELINE];
  }

  cxt[LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0;
  return rc;
}

}
