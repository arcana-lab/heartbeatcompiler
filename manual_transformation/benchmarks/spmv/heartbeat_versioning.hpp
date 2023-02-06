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

void spmv_heartbeat_versioning(double *, uint64_t *, uint64_t *, double *, double *, uint64_t);
void HEARTBEAT_loop0(double *, uint64_t *, uint64_t *, double *, double *, uint64_t);
void HEARTBEAT_loop1(double *, uint64_t *, uint64_t *, double *, uint64_t, double &);
int64_t HEARTBEAT_loop_1_0_leftover(uint64_t *, uint64_t, uint64_t *);

static bool run_heartbeat = true;
uint64_t *constLiveIns;

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

  return HEARTBEAT_loop1_slice(cxts, myIndex, 0, 0, startIter, maxIter);

#endif

}
