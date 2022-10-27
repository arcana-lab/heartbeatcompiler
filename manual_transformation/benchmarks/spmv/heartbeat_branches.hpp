#pragma once

#include "loop_handler.hpp"

#ifdef CHUNK_LOOP_ITERATIONS
  #ifndef CHUNKSIZE_1
    #define CHUNKSIZE_1 1024
  #endif
#endif

namespace spmv {

void spmv_heartbeat_versioning(double *, uint64_t *, uint64_t *, double *, double *, uint64_t);
void HEARTBEAT_loop0(double *, uint64_t *, uint64_t *, double *, double *, uint64_t);
void HEARTBEAT_loop1(double *, uint64_t *, uint64_t *, double *, uint64_t, double &);
uint64_t HEARTBEAT_loop0_cloned(uint64_t *, uint64_t *, void **, void *, uint64_t, uint64_t, taskparts::future *&);
uint64_t HEARTBEAT_loop1_cloned(uint64_t *, uint64_t *, void **, void *, uint64_t, uint64_t, taskparts::future *&);

typedef uint64_t(*functionPointer)(uint64_t *, uint64_t *, void **, void *, uint64_t, uint64_t, taskparts::future *&);
functionPointer splittingTasks[2] = {
  &HEARTBEAT_loop0_cloned,
  &HEARTBEAT_loop1_cloned
};

static bool run_heartbeat = true;

// Entry function for the benchmark
void spmv_heartbeat_branches(double *val, uint64_t *row_ptr, uint64_t *col_ind, double *x, double *y, uint64_t n) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate the startIterations and maxIterations array
    uint64_t startIterations[2 * 8];
    uint64_t maxIterations[2 * 8];

    // allocate live-in environments
    uint64_t *liveInEnvironments[2 * 8];

    // allocate live-in environment for loop0
    uint64_t liveInEnvironmentForLoop0[5 * 8];
    liveInEnvironments[0 * 8] = (uint64_t *)liveInEnvironmentForLoop0;

    // no live-out environment for loop0

    // store into live-in environment
    liveInEnvironmentForLoop0[0 * 8] = (uint64_t)val;
    liveInEnvironmentForLoop0[1 * 8] = (uint64_t)row_ptr;
    liveInEnvironmentForLoop0[2 * 8] = (uint64_t)col_ind;
    liveInEnvironmentForLoop0[3 * 8] = (uint64_t)x;
    liveInEnvironmentForLoop0[4 * 8] = (uint64_t)y;
    
    // set the start and max iteration for loop0
    startIterations[0 * 8] = 0;
    maxIterations[0 * 8] = n;

    // allocate future pointer
    taskparts::future *fut = nullptr;

    // invoke loop0 in heartbeat form
    HEARTBEAT_loop0_cloned(startIterations, maxIterations, (void **)liveInEnvironments, nullptr, 0, 0, fut);

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
uint64_t HEARTBEAT_loop0_cloned(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, void *liveOutEnvironment, uint64_t myLevel, uint64_t myIndex, taskparts::future *&fut) {
  // allocate live-in environment for loop1
  uint64_t liveInEnvironmentForLoop1[3 * 8];
  ((uint64_t **)liveInEnvironments)[(myLevel + 1) * 8] = (uint64_t *)liveInEnvironmentForLoop1;

  // allocate live-out environment for loop1
  uint64_t liveOutEnvironmentForLoop1[1 * 8];

  // allocate reduction array for loop1
  double reductionArrayLiveOut1Loop1[1 * 8];
  liveOutEnvironmentForLoop1[0 * 8] = (uint64_t)reductionArrayLiveOut1Loop1;

  // load live-in environment
  uint64_t *liveInEnvironment = ((uint64_t **)liveInEnvironments)[myLevel * 8];
  double *val = (double *)liveInEnvironment[0 * 8];
  uint64_t *row_ptr = (uint64_t *)liveInEnvironment[1 * 8];
  uint64_t *col_ind = (uint64_t *)liveInEnvironment[2 * 8];
  double *x = (double *)liveInEnvironment[3 * 8];
  double *y = (double *)liveInEnvironment[4 * 8];

  // store into loop1's live-in environment
  liveInEnvironmentForLoop1[0 * 8] = (uint64_t)val;
  liveInEnvironmentForLoop1[1 * 8] = (uint64_t)col_ind;
  liveInEnvironmentForLoop1[2 * 8] = (uint64_t)x;

  // allocate returnLevel variable and initialize
  uint64_t returnLevel = 1 + 1;

  for (; startIterations[myLevel] < maxIterations[myLevel]; startIterations[myLevel]++) {
    int rc = loop_handler(startIterations, maxIterations, liveInEnvironments, nullptr, myLevel, myIndex, returnLevel, splittingTasks, fut);
    if (rc == 1) {
      goto reduction;
    }

    double r = 0.0;

    // set the start and max iteration for loop0
    startIterations[(myLevel + 1) * 8] = row_ptr[startIterations[myLevel]];
    maxIterations[(myLevel + 1) * 8] = row_ptr[startIterations[myLevel] + 1];
    returnLevel = HEARTBEAT_loop1_cloned(startIterations, maxIterations, liveInEnvironments, (void *)liveOutEnvironmentForLoop1, myLevel + 1, 0, fut);

    y[startIterations[myLevel]] = r + reductionArrayLiveOut1Loop1[0 * 8];
  }

  if (returnLevel == myLevel) {
reduction:
    fut->force();
  }

  // synchronization point for the splitting task
  return returnLevel;
}

uint64_t HEARTBEAT_loop1_cloned(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, void *liveOutEnvironment, uint64_t myLevel, uint64_t myIndex, taskparts::future *&fut) {
  // allocate live-out environment for next level
  uint64_t liveOutEnvironmentNextLevel[1 * 8];

  // allocate reduction array for next level
  double reductionArrayLiveOut1NextLevel[2 * 8];
  liveOutEnvironmentNextLevel[0 * 8] = (uint64_t)reductionArrayLiveOut1NextLevel;

  // load live-in environment
  uint64_t *liveInEnvironment = ((uint64_t **)liveInEnvironments)[myLevel * 8];
  double *val = (double *)liveInEnvironment[0 * 8];
  uint64_t *col_ind = (uint64_t *)liveInEnvironment[1 * 8];
  double *x = (double *)liveInEnvironment[2 * 8];

  // initialize my private copy of reduction array
  double *reductionArrayLiveOut1 = (double *)((uint64_t *)liveOutEnvironment)[0 * 8];
  reductionArrayLiveOut1[myIndex * 8] = 0.0;

  // allocate returnLevel variable and initialize
  uint64_t returnLevel = 1 + 1;

  double r_private = 0.0;
#if defined(CHUNK_LOOP_ITERATIONS)
  for (; ;) {
    int rc = loop_handler(startIterations, maxIterations, liveInEnvironments, (void *)liveOutEnvironmentNextLevel, myLevel, myIndex, returnLevel, splittingTasks, fut);
    if (rc == 1) {
      if (returnLevel < myLevel) {
        goto leftover;
      }
      goto reduction;
    }

    uint64_t low = startIterations[myLevel * 8];
    uint64_t high = std::min(maxIterations[myLevel * 8], startIterations[myLevel * 8] + CHUNKSIZE_1);

    for (uint64_t k = low; k < high; k++) {
      r_private += val[k] * x[col_ind[k]];
    }

    startIterations[myLevel * 8] = high;
    if (!(startIterations[myLevel * 8] < maxIterations[myLevel * 8])) {
      break;
    }
  }
#else
  for (; startIterations[myLevel * 8] < maxIterations[myLevel * 8]; startIterations[myLevel * 8]++) {
    int rc = loop_handler(startIterations, maxIterations, liveInEnvironments, (void *)liveOutEnvironmentNextLevel, myLevel, myIndex, returnLevel, splittingTasks, fut);
    if (rc == 1) {
      if (returnLevel < myLevel) {
        goto leftover;
      }
      goto reduction;
    }

    r_private += val[startIterations[myLevel * 8]] * x[col_ind[startIterations[myLevel * 8]]];
  }
#endif

  if (returnLevel == myLevel) {
reduction:
    // synchronization point for the leftover task (either splitting at the same level and return from a higher nesting level)
    fut->force();
    reductionArrayLiveOut1[myIndex * 8] += r_private + reductionArrayLiveOut1NextLevel[0] + reductionArrayLiveOut1NextLevel[1 * 8];
    return returnLevel;
  }

  // synchronization point for the splitting task
  reductionArrayLiveOut1[myIndex * 8] += r_private;
  return returnLevel;

leftover:
  for (uint64_t k = startIterations[myLevel * 8]; k < maxIterations[myLevel * 8]; k++) {
    r_private += val[k] * x[col_ind[k]];
  }
  startIterations[myLevel * 8] = maxIterations[myLevel * 8];

  // reduction
  reductionArrayLiveOut1[myIndex * 8] += r_private;
  return returnLevel;
}

}