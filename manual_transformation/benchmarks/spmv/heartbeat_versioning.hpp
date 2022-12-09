#pragma once

#include "loop_handler.hpp"

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
uint64_t HEARTBEAT_loop0_cloned(uint64_t *, uint64_t *, uint64_t *, uint64_t **, uint64_t **, uint64_t, uint64_t, uint64_t);
uint64_t HEARTBEAT_loop1_cloned(uint64_t *, uint64_t *, uint64_t *, uint64_t **, uint64_t **, uint64_t, uint64_t, uint64_t);
uint64_t HEARTBEAT_loop1_leftover(uint64_t *, uint64_t *, uint64_t *, uint64_t **, uint64_t **, uint64_t, uint64_t, uint64_t);
uint64_t HEARTBEAT_loop1_optimized(uint64_t *, uint64_t *, uint64_t *, uint64_t *, uint64_t *, uint64_t);

typedef uint64_t(*functionPointer)(uint64_t *, uint64_t *, uint64_t *, uint64_t **, uint64_t **, uint64_t, uint64_t, uint64_t);
functionPointer splittingTasks[2] = {
  &HEARTBEAT_loop0_cloned,
  &HEARTBEAT_loop1_cloned
};
functionPointer leftoverTasks[1] = {
  &HEARTBEAT_loop1_leftover
};
typedef uint64_t(*leafFunctionPointer)(uint64_t *, uint64_t *, uint64_t *, uint64_t *, uint64_t *, uint64_t);
leafFunctionPointer leafTasks[1] = {
  &HEARTBEAT_loop1_optimized
};

static bool run_heartbeat = true;

// Entry function for the benchmark
void spmv_heartbeat_versioning(double *val, uint64_t *row_ptr, uint64_t *col_ind, double *x, double *y, uint64_t n) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate const live-ins
    uint64_t constLiveIns[5];
    constLiveIns[0] = (uint64_t)row_ptr;
    constLiveIns[1] = (uint64_t)y;
    constLiveIns[2] = (uint64_t)val;
    constLiveIns[3] = (uint64_t)col_ind;
    constLiveIns[4] = (uint64_t)x;

    // allocate the startIters and maxIters array
    uint64_t startIters[2 * 8];
    uint64_t maxIters[2 * 8];

    // allocate live-in and live-out environments
    uint64_t *liveInEnvs[2 * 8];
    uint64_t *liveOutEnvs[2 * 8];

    // set the start and max iteration for loop0
    startIters[0 * 8] = 0;
    maxIters[0 * 8] = n;

    // invoke loop0 in heartbeat form
    HEARTBEAT_loop0_cloned(startIters, maxIters, constLiveIns, liveInEnvs, liveOutEnvs, 0, 0, 0);

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
uint64_t HEARTBEAT_loop0_cloned(uint64_t *startIters, uint64_t *maxIters, uint64_t *constLiveIns, uint64_t **liveInEnvs, uint64_t **liveOutEnvs, uint64_t myLevel, uint64_t myIndex, uint64_t startingLevel) {
  // load const live-ins
  uint64_t *row_ptr = (uint64_t *)constLiveIns[0];
  double *y = (double *)constLiveIns[1];

  // allocate live-out environment for loop1
  uint64_t liveOutEnvLoop1[1 * 8];
  liveOutEnvs[(myLevel + 1) * 8] = liveOutEnvLoop1;

  // allocate reduction array for loop1
  double redArrLiveOut0Loop1[1 * 8];
  liveOutEnvLoop1[0 * 8] = (uint64_t)redArrLiveOut0Loop1;

#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t low, high;
  for (; ;) {
    low = startIters[myLevel * 8];
    high = std::min(maxIters[myLevel * 8], startIters[myLevel * 8] + CHUNKSIZE_0);
    startIters[myLevel * 8] = high - 1;

    for (; low < high; low++) {
      double r = 0.0;

      // set the start and max iteration for loop1
      startIters[(myLevel + 1) * 8] = row_ptr[low];
      maxIters[(myLevel + 1) * 8] = row_ptr[low + 1];

      HEARTBEAT_loop1_cloned(startIters, maxIters, constLiveIns, liveInEnvs, liveOutEnvs, myLevel + 1, 0, startingLevel);

      y[low] = r + redArrLiveOut0Loop1[0 * 8];
    }

    if (low == maxIters[myLevel]) {
      break;
    }
    loop_handler(startIters, maxIters, constLiveIns, liveInEnvs, liveOutEnvs, myLevel, startingLevel, 2, splittingTasks, leftoverTasks, nullptr);
    if (low == maxIters[myLevel]) {
      break;
    }
    startIters[myLevel * 8] = low;
  }
#else
  for (; startIters[myLevel] < maxIters[myLevel]; startIters[myLevel]++) {
    double r = 0.0;

    // set the start and max iteration for loop1
    startIters[(myLevel + 1) * 8] = row_ptr[startIters[myLevel]];
    maxIters[(myLevel + 1) * 8] = row_ptr[startIters[myLevel] + 1];

    HEARTBEAT_loop1_cloned(startIters, maxIters, constLiveIns, liveInEnvs, liveOutEnvs, myLevel + 1, 0, startingLevel);

    y[startIters[myLevel]] = r + redArrLiveOut0Loop1[0 * 8];
    loop_handler(startIters, maxIters, constLiveIns, liveInEnvs, liveOutEnvs, myLevel, startingLevel, 2, splittingTasks, leftoverTasks, nullptr);
  }
#endif

  return LLONG_MAX;
}

uint64_t HEARTBEAT_loop1_cloned(uint64_t *startIters, uint64_t *maxIters, uint64_t *constLiveIns, uint64_t **liveInEnvs, uint64_t **liveOutEnvs, uint64_t myLevel, uint64_t myIndex, uint64_t startingLevel) {
  // load const live-ins
  double *val = (double *)constLiveIns[2];
  uint64_t *col_ind = (uint64_t *)constLiveIns[3];
  double *x = (double *)constLiveIns[4];

  // initialize my private copy of reduction array
  uint64_t *liveOutEnv = liveOutEnvs[myLevel * 8];
  double *redArrLiveOut0 = (double *)liveOutEnv[0 * 8];
  redArrLiveOut0[myIndex * 8] = 0.0;

  // allocate live-out environment for kids
  uint64_t liveOutEnvKids[1 * 8];
  liveOutEnvs[myLevel * 8] = liveOutEnvKids;

  // allocate reduction array for kids
  double redArrLiveOut0Kids[2 * 8];
  liveOutEnvKids[0 * 8] = (uint64_t)redArrLiveOut0Kids;

  uint64_t rc = LLONG_MAX;
  double r_private = 0.0;
#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t low, high;
  for (; ;) {
    low = startIters[myLevel * 8];
    high = std::min(maxIters[myLevel * 8], startIters[myLevel * 8] + CHUNKSIZE_1);

    for (; low < high; low++) {
      r_private += val[low] * x[col_ind[low]];
    }

    if (low == maxIters[myLevel * 8]) {
      break;
    }
    startIters[myLevel * 8] = low;
    rc = loop_handler(startIters, maxIters, constLiveIns, liveInEnvs, liveOutEnvs, myLevel, startingLevel, 2, splittingTasks, leftoverTasks, leafTasks);
  }
#else
  for (; startIters[myLevel * 8] < maxIters[myLevel * 8]; startIters[myLevel * 8]++) {
    r_private += val[startIters[myLevel * 8]] * x[col_ind[startIters[myLevel * 8]]];
    rc = loop_handler(startIters, maxIters, constLiveIns, liveInEnvs, liveOutEnvs, myLevel, startingLevel, 2, splittingTasks, leftoverTasks, leafTasks);
  }
#endif

  // reduction
  if (rc == LLONG_MAX) {      // either no heartbeat promotion happens or promotion happens at a higher nested level
    redArrLiveOut0[myIndex * 8] += r_private;
  } else if (rc == myLevel) { // heartbeat promotion happens at my level
    redArrLiveOut0[myIndex * 8] += r_private + redArrLiveOut0Kids[0 * 8] + redArrLiveOut0Kids[1 * 8];
    rc = LLONG_MAX;
    goto exit;
  } else {                    // heartbeat promotion happens and splitting happens at a lower nested level
    redArrLiveOut0[myIndex * 8] += r_private + redArrLiveOut0Kids[0 * 8];
  }

exit:
  // reset live-out environment
  liveOutEnvs[myLevel * 8] = liveOutEnv;

  return rc;
}

// Leftover loops
uint64_t HEARTBEAT_loop1_leftover(uint64_t *startIters, uint64_t *maxIters, uint64_t *constLiveIns, uint64_t **liveInEnvs, uint64_t **liveOutEnvs, uint64_t myLevel, uint64_t myIndex, uint64_t startingLevel) {
#ifndef LEFTOVER_SPLITTABLE

  // load const live-ins
  double *val = (double *)constLiveIns[2];
  uint64_t *col_ind = (uint64_t *)constLiveIns[3];
  double *x = (double *)constLiveIns[4];

  // initialize my private copy of reduction array
  uint64_t *liveOutEnv = liveOutEnvs[myLevel * 8];
  double *redArrLiveOut0 = (double *)liveOutEnv[0 * 8];
  redArrLiveOut0[myIndex * 8] = 0.0;

  double r_private = 0.0;
  for (uint64_t k = startIters[myLevel * 8]; k < maxIters[myLevel * 8]; k++) {
    r_private += val[k] * x[col_ind[k]];
  }
  startIters[myLevel * 8] = maxIters[myLevel * 8];

  // reduction
  redArrLiveOut0[myIndex * 8] += r_private;

  return 0;

#else

  return HEARTBEAT_loop1_cloned(startIters, maxIters, constLiveIns, liveInEnvs, liveOutEnvs, myLevel, myIndex, startingLevel);

#endif

}

// Cloned optimized leaf loops
uint64_t HEARTBEAT_loop1_optimized(uint64_t *startIter, uint64_t *maxIter, uint64_t *constLiveIns, uint64_t *liveInEnv, uint64_t *liveOutEnv, uint64_t myIndex) {
  // load const live-ins
  double *val = (double *)constLiveIns[2];
  uint64_t *col_ind = (uint64_t *)constLiveIns[3];
  double *x = (double *)constLiveIns[4];

  // initialize my private copy of reduction array
  double *redArrLiveOut0 = (double *)liveOutEnv[0 * 8];
  redArrLiveOut0[myIndex * 8] = 0.0;

  // allocate live-out environment for kids
  uint64_t liveOutEnvKids[1 * 8];

  // allocate reduction array for kids
  double redArrLiveOut0Kids[2 * 8];
  liveOutEnvKids[0 * 8] = (uint64_t)redArrLiveOut0Kids;

  uint64_t rc = LLONG_MAX;
  double r_private = 0.0;
#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t low, high;
  for (; ;) {
    low = startIter[0 * 8];
    high = std::min(maxIter[0 * 8], startIter[0 * 8] + CHUNKSIZE_1);

    for (; low < high; low++) {
      r_private += val[low] * x[col_ind[low]];
    }

    if (low == maxIter[0 * 8]) {
      break;
    }
    startIter[0 * 8] = low;
    rc = loop_handler_optimized(startIter, maxIter, constLiveIns, liveInEnv, liveOutEnvKids, &HEARTBEAT_loop1_optimized);
  }
#else
  for (; startIter[0 * 8] < maxIter[0 * 8]; startIter[0 * 8]++) {
    r_private += val[startIter[0 * 8]] * x[col_ind[startIter[0 * 8]]];
    rc = loop_handler_optimized(startIter, maxIter, constLiveIns, liveInEnv, liveOutEnvKids, &HEARTBEAT_loop1_optimized);
  }
#endif

  // reduction
  if (rc == LLONG_MAX) {      // no heartbeat promotion happens
    redArrLiveOut0[myIndex * 8] += r_private;
  } else if (rc == 0) {       // heartbeat promotion happens
    redArrLiveOut0[myIndex * 8] += r_private + redArrLiveOut0Kids[0 * 8] + redArrLiveOut0Kids[1 * 8];
  }

  return rc;
}

}
