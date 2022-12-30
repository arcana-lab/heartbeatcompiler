#pragma once

#include "loop_handler.hpp"

#define SUB(array, row_sz, i, j) (array[i * row_sz + j])

#define CACHELINE     8
#define LIVE_IN_ENV   0
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

namespace floyd_warshall {

void floyd_warshall_heartbeat_versioning(int *, int);
void HEARTBEAT_loop0(int *, int, int);
void HEARTBEAT_loop1(int *, int, int, int);

int64_t HEARTBEAT_loop0_slice(uint64_t *, uint64_t, uint64_t);
int64_t HEARTBEAT_loop1_slice(uint64_t *, uint64_t, uint64_t, uint64_t, uint64_t);
__inline__ int64_t HEARTBEAT_loop0_slice_variadic(uint64_t *cxts, uint64_t startIter, uint64_t maxIter, ...) {
  return HEARTBEAT_loop0_slice(cxts, startIter, maxIter);
}
__inline__ int64_t HEARTBEAT_loop1_slice_variadic(uint64_t *cxts, uint64_t startIter, uint64_t maxIter, ...) {
  return HEARTBEAT_loop1_slice(cxts, startIter, maxIter, 0, 0);
}
typedef int64_t(*sliceTaskPointer)(uint64_t *, uint64_t, uint64_t, ...);
sliceTaskPointer sliceTasks[2] = {
  &HEARTBEAT_loop0_slice_variadic,
  &HEARTBEAT_loop1_slice_variadic
};

int64_t HEARTBEAT_loop_1_0_leftover(uint64_t *, uint64_t *);
typedef int64_t(*leftoverTaskPointer)(uint64_t *, uint64_t *);
leftoverTaskPointer leftoverTasks[1] = {
  &HEARTBEAT_loop_1_0_leftover
};

int64_t HEARTBEAT_loop1_optimized(uint64_t *, uint64_t, uint64_t);
typedef int64_t(*leafTaskPointer)(uint64_t *, uint64_t, uint64_t);
leafTaskPointer leafTasks[1] = {
  &HEARTBEAT_loop1_optimized
};

static bool run_heartbeat = true;
static uint64_t *constLiveIns;

// Entry function for the benchmark
void floyd_warshall_heartbeat_versioning(int *dist, int vertices) {
  for (int via = 0; via < vertices; via++) {
    HEARTBEAT_loop0(dist, vertices, via);
  }
}

// Outlined loops
void HEARTBEAT_loop0(int *dist, int vertices, int via) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // initialize number of levels for this nested loop
    numLevels = NUM_LEVELS;

    // allocate const live-ins
    constLiveIns = (uint64_t *)alloca(sizeof(uint64_t) * 3);
    constLiveIns[0] = (uint64_t)dist;
    constLiveIns[1] = (uint64_t)vertices;
    constLiveIns[2] = (uint64_t)via;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS * CACHELINE];

    // invoke loop0 in heartbeat form
    HEARTBEAT_loop0_slice(cxts, 0, (uint64_t)vertices);

    run_heartbeat = true;
  } else {
    for (int from = 0; from < vertices; from++) {
      HEARTBEAT_loop1(dist, vertices, via, from);
    }
  }

  return;
}

void HEARTBEAT_loop1(int *dist, int vertices, int via, int from) {
  for (int to = 0; to < vertices; to++) {
    if ((from != to) && (from != via) && (to != via)) {
      SUB(dist, vertices, from, to) =
        std::min(SUB(dist, vertices, from, to),
                SUB(dist, vertices, from, via) + SUB(dist, vertices, via, to));
    }
  }

  return;
}

// Cloned loops
int64_t HEARTBEAT_loop0_slice(uint64_t *cxts, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  int vertices = (int)constLiveIns[1];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  int low, high;

  // store into live-in environment for loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&low;

  for (; ;) {
    low = (int)startIter;
    high = (int)std::min(maxIter, startIter + CHUNKSIZE_0);

    for (; low < high; low++) {
      rc = HEARTBEAT_loop1_slice(cxts, 0, (uint64_t)vertices, low, maxIter);
      if (rc > 0) {
        high = low + 1;
      }
    }

    // exit the chunk execution when either
    // 1. heartbeat promotion happens at a higher nested level and in the process of returnning
    // 2. all iterations are finished
    if (rc > 0 || low == (int)maxIter) {
      break;
    }

    rc = loop_handler(cxts, LEVEL_ZERO, sliceTasks, leftoverTasks, nullptr, (uint64_t)(low - 1), maxIter);
    if (rc > 0) {
      break;
    }
    startIter = low;
  }
#else
  // store into live-in environment for loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&startIter;

  for (; (int)startIter < (int)maxIter; startIter++) {
    rc = HEARTBEAT_loop1_slice(cxts, 0, (uint64_t)vertices, startIter, maxIter);
    if (rc > 0) {
      maxIter = startIter + 1;
    }

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

int64_t HEARTBEAT_loop1_slice(uint64_t *cxts, uint64_t startIter, uint64_t maxIter, uint64_t startIter0, uint64_t maxIter0) {
  // load const live-ins
  int *dist = (int *)constLiveIns[0];
  int vertices = (int)constLiveIns[1];
  int via = (int)constLiveIns[2];

  // load live-in environment
  int from = *(int *)cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  int low, high;
  for (; ;) {
    low = (int)startIter;
    high = (int)std::min(maxIter, startIter + CHUNKSIZE_1);

    for (; low < high; low++) {
      if ((from != low) && (from != via) && (low != via)) {
        SUB(dist, vertices, from, low) =
          std::min(SUB(dist, vertices, from, low),
                  SUB(dist, vertices, from, via) + SUB(dist, vertices, via, low));
      }
    }
    if (low == (int)maxIter) {
      break;
    }

    rc = loop_handler(cxts, LEVEL_ONE, sliceTasks, leftoverTasks, leafTasks, startIter0, maxIter0, uint64_t(low - 1), maxIter);
    if (rc > 0) {
      break;
    }
    startIter = low;
  }
#else
  for (; (int)startIter < (int)maxIter; startIter++) {
    if ((from != (int)startIter) && (from != via) && ((int)startIter != via)) {
      SUB(dist, vertices, from, (int)startIter) =
        std::min(SUB(dist, vertices, from, (int)startIter),
                SUB(dist, vertices, from, via) + SUB(dist, vertices, via, (int)startIter));
    }
    rc = loop_handler(cxts, LEVEL_ONE, sliceTasks, leftoverTasks, leafTasks, startIter0, maxIter0, startIter, maxIter);
    if (rc > 0) {
      maxIter = startIter + 1;
    }
  }
#endif

  return rc - 1;
}

// Leftover loops
int64_t HEARTBEAT_loop_1_0_leftover(uint64_t *cxts, uint64_t *itersArr) {
  // load startIter and maxIter
  uint64_t startIter = itersArr[LEVEL_ONE * 2 + START_ITER];
  uint64_t maxIter   = itersArr[LEVEL_ONE * 2 + MAX_ITER];

#ifndef LEFTOVER_SPLITTABLE

  // load const live-ins
  int *dist = (int *)constLiveIns[0];
  int vertices = (int)constLiveIns[1];
  int via = (int)constLiveIns[2];

  // load live-in environment
  int from = *(int *)cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV];

  for (int to = (int)startIter; to < (int)maxIter; to++) {
    if ((from != to) && (from != via) && (to != via)) {
      SUB(dist, vertices, from, to) =
        std::min(SUB(dist, vertices, from, to),
                SUB(dist, vertices, from, via) + SUB(dist, vertices, via, to));
    }
  }

  return 0;

#else

  return HEARTBEAT_loop1_slice(cxts, startIter, maxIter, 0, 0);

#endif
}

// Cloned optimized leaf loops
int64_t HEARTBEAT_loop1_optimized(uint64_t *cxt, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  int *dist = (int *)constLiveIns[0];
  int vertices = (int)constLiveIns[1];
  int via = (int)constLiveIns[2];

  // load live-in environment
  int from = *(int *)cxt[LIVE_IN_ENV];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  int low, high;
  for (; ;) {
    low = (int)startIter;
    high = (int)std::min(maxIter, startIter + CHUNKSIZE_1);

    for (; low < high; low++) {
      if ((from != low) && (from != via) && (low != via)) {
        SUB(dist, vertices, from, low) =
          std::min(SUB(dist, vertices, from, low),
                  SUB(dist, vertices, from, via) + SUB(dist, vertices, via, low));
      }
    }
    if (low == (int)maxIter) {
      break;
    }

    rc = loop_handler_optimized(cxt, (uint64_t)(low - 1), maxIter, &HEARTBEAT_loop1_optimized);
    if (rc > 0) {
      break;
    }
    startIter = low;
  }
#else
  for (; (int)startIter < (int)maxIter; startIter++) {
    if ((from != (int)startIter) && (from != via) && ((int)startIter != via)) {
      SUB(dist, vertices, from, (int)startIter) =
        std::min(SUB(dist, vertices, from, (int)startIter),
                SUB(dist, vertices, from, via) + SUB(dist, vertices, via, (int)startIter));
    }
    rc = loop_handler_optimized(cxt, startIter, maxIter, &HEARTBEAT_loop1_optimized);
    if (rc > 0) {
      maxIter = startIter + 1;
    }
  }
#endif

  return rc - 1;
}

}
