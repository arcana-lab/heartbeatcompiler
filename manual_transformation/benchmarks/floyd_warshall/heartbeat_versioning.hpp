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

void floyd_warshall_heartbeat_versioning(int *, int);
void HEARTBEAT_loop0(int *, int, int);
void HEARTBEAT_loop1(int *, int, int, int);
int64_t HEARTBEAT_loop_1_0_leftover(uint64_t *, uint64_t *);

static bool run_heartbeat = true;
uint64_t *constLiveIns;

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

  return HEARTBEAT_loop1_slice(cxts, 0, 0, startIter, maxIter);

#endif
}
