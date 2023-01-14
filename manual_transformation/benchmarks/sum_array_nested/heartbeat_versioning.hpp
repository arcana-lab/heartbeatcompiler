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

void sum_array_nested_heartbeat_versioning(char **, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t &);
void HEARTBEAT_loop0(char **, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t &);
void HEARTBEAT_loop1(char **, uint64_t, uint64_t, uint64_t, uint64_t &);

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
void sum_array_nested_heartbeat_versioning(char **a, uint64_t low1, uint64_t high1, uint64_t low2, uint64_t high2, uint64_t &result) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // initialize number of levels for this nested loop
    numLevels = NUM_LEVELS;

    // allocate const live-ins
    constLiveIns = (uint64_t *)alloca(sizeof(uint64_t) * 3);
    constLiveIns[0] = (uint64_t)a;
    constLiveIns[1] = (uint64_t)low2;
    constLiveIns[2] = (uint64_t)high2;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS * CACHELINE];

    // allocate reduction array (as live-out environment) for loop0
    uint64_t redArrLiveOut0Loop0[1 * CACHELINE];
    cxts[LEVEL_ZERO * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0Loop0;

    // invoke loop0 in heartbeat form
    HEARTBEAT_loop0_slice(cxts, 0, low1, high1);
    result += redArrLiveOut0Loop0[0 * 8];

    run_heartbeat = true;
  } else {
    uint64_t r0 = 0;
    HEARTBEAT_loop0(a, low1, high1, low2, high2, r0);
    result += r0;
  }

  return;
}

// Outlined loops
void HEARTBEAT_loop0(char **a, uint64_t low1, uint64_t high1, uint64_t low2, uint64_t high2, uint64_t &r0) {
  for (uint64_t i = low1; i < high1; i++) {
    uint64_t r1 = 0;
    HEARTBEAT_loop1(a, i, low2, high2, r1);
    r0 += r1;
  }

  return;
}

void HEARTBEAT_loop1(char **a, uint64_t i, uint64_t low2, uint64_t high2, uint64_t &r1) {
  for (uint64_t j = low2; j < high2; j++) {
    r1 += a[i][j];
  }

  return;
}

// Cloned loops
int64_t HEARTBEAT_loop0_slice(uint64_t *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  char **a = (char **)constLiveIns[0];
  uint64_t low2 = (uint64_t)constLiveIns[1];
  uint64_t high2 = (uint64_t)constLiveIns[2];

  // initialize my private copy of reduction array
  uint64_t *redArrLiveOut0 = (uint64_t *)cxts[LEVEL_ZERO * CACHELINE + LIVE_OUT_ENV];
  redArrLiveOut0[myIndex * CACHELINE] = 0;

  // allocate reduction array (as live-out environment) for loop1
  uint64_t redArrLiveOut0Loop1[1 * CACHELINE];
  cxts[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0Loop1;

  // allocate reduction array (as live-out environment) for kids
  uint64_t redArrLiveOut0Kids[2 * CACHELINE];
  cxts[LEVEL_ZERO * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0Kids;

  int64_t rc = 0;
  uint64_t r_private = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t low, high;

  // store into live-in environment for loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&low;

  for (; ;) {
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

    rc = loop_handler(cxts, LEVEL_ZERO, leftoverTasks, nullptr, low - 1, maxIter, 0, 0);
    if (rc > 0) {
      break;
    }
    startIter = low;
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
    rc = loop_handler(cxts, LEVEL_ZERO, leftoverTasks, nullptr, startIter, maxIter, 0, 0);
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
  cxts[LEVEL_ZERO * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0;

  return rc - 1;
}

int64_t HEARTBEAT_loop1_slice(uint64_t *cxts, uint64_t myIndex, uint64_t startIter0, uint64_t maxIter0, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  char **a = (char **)constLiveIns[0];

  // load live-in environment
  uint64_t i = *(uint64_t *)cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV];

  // initialize my private copy of reduction array
  uint64_t *redArrLiveOut0 = (uint64_t *)cxts[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV];
  redArrLiveOut0[myIndex * CACHELINE] = 0;

  // allocate reduction array (as live-out environment) for kids
  uint64_t redArrLiveOut0Kids[2 * CACHELINE];
  cxts[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0Kids;

  int64_t rc = 0;
  uint64_t r_private = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t low, high;
  for (; ;) {
    low = startIter;
    high = std::min(maxIter, startIter + CHUNKSIZE_1);

    for (; low < high; low++) {
      r_private += a[i][low];
    }
    if (low == maxIter) {
      break;
    }

    rc = loop_handler(cxts, LEVEL_ONE, leftoverTasks, leafTasks, startIter0, maxIter0, low - 1, maxIter);
    if (rc > 0) {
      break;
    }
    startIter = low;
  }
#else
  for (; startIter < maxIter; startIter++) {
    r_private += a[i][startIter];
    rc = loop_handler(cxts, LEVEL_ONE, leftoverTasks, leafTasks, startIter0, maxIter0, startIter, maxIter);
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

// Leftover loops
int64_t HEARTBEAT_loop_1_0_leftover(uint64_t *cxts, uint64_t myIndex, uint64_t *itersArr) {
  // load startIter and maxIter
  uint64_t startIter = itersArr[LEVEL_ONE * 2 + START_ITER];
  uint64_t maxIter   = itersArr[LEVEL_ONE * 2 + MAX_ITER];

#ifndef LEFTOVER_SPLITTABLE

  // load const live-ins
  char **a = (char **)constLiveIns[0];

  // load live-in environment
  uint64_t i = *(uint64_t *)cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV];

  // initialize my private copy of reduction array
  uint64_t *redArrLiveOut0 = (uint64_t *)cxts[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV];
  redArrLiveOut0[myIndex * CACHELINE] = 0;

  uint64_t r_private = 0;
  for (; startIter < maxIter; startIter++) {
    r_private += a[i][startIter];
  }

  // reduction
  redArrLiveOut0[myIndex * CACHELINE] += r_private;

  return 0;

#else

  return HEARTBEAT_loop1_slice(cxts, myIndex, 0, 0, startIter, maxIter);

#endif

}

// Cloned optimized leaf loops
int64_t HEARTBEAT_loop1_optimized(uint64_t *cxt, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  char **a = (char **)constLiveIns[0];

  // load live-in environment
  uint64_t i = *(uint64_t *)cxt[LIVE_IN_ENV];

  // initialize my private copy of reduction array
  uint64_t *redArrLiveOut0 = (uint64_t *)cxt[LIVE_OUT_ENV];
  redArrLiveOut0[myIndex * CACHELINE] = 0;

  // allocate reduction array (as live-out environment) for kids
  uint64_t redArrLiveOut0Kids[2 * CACHELINE];
  cxt[LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0Kids;

  int64_t rc = 0;
  uint64_t r_private = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t low, high;
  for (; ;) {
    low = startIter;
    high = std::min(maxIter, startIter + CHUNKSIZE_1);

    for (; low < high; low++) {
      r_private += a[i][low];
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
    r_private += a[i][startIter];
    rc = loop_handler_optimized(cxt, startIter, maxIter, &HEARTBEAT_loop1_optimized);
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
