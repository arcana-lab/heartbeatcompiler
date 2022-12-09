#pragma once

#include "loop_handler.hpp"

#define SUB(array, row_sz, i, j) (array[i * row_sz + j])

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
void HEARTBEAT_loop0_cloned(uint64_t *, uint64_t *, uint64_t *, uint64_t **, uint64_t, uint64_t);
void HEARTBEAT_loop1_cloned(uint64_t *, uint64_t *, uint64_t *, uint64_t **, uint64_t, uint64_t);
void HEARTBEAT_loop1_leftover(uint64_t *, uint64_t *, uint64_t *, uint64_t **, uint64_t, uint64_t);
void HEARTBEAT_loop1_optimized(uint64_t *, uint64_t *, uint64_t *, uint64_t *);

typedef void(*functionPointer)(uint64_t *, uint64_t *, uint64_t *, uint64_t **, uint64_t, uint64_t);
functionPointer splittingTasks[2] = {
  &HEARTBEAT_loop0_cloned,
  &HEARTBEAT_loop1_cloned
};
functionPointer leftoverTasks[1] = {
  &HEARTBEAT_loop1_leftover
};
typedef void(*leafFunctionPointer)(uint64_t *, uint64_t *, uint64_t *, uint64_t *);
leafFunctionPointer leafTasks[1] = {
  &HEARTBEAT_loop1_optimized
};

static bool run_heartbeat = true;
const static uint64_t numLevels = 2;

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

    // allocate const live-ins
    uint64_t constLiveIns[3];
    constLiveIns[0] = (uint64_t)dist;
    constLiveIns[1] = (uint64_t)vertices;
    constLiveIns[2] = (uint64_t)via;

    // allocate the startIters and maxIters array
    uint64_t startIters[2 * 8];
    uint64_t maxIters[2 * 8];

    // allocate live-in environments
    uint64_t *liveInEnvs[2 * 8];

    // set the start and max iteration for loop0
    startIters[0 * 8] = 0;
    maxIters[0 * 8] = (uint64_t)vertices;

    // invoke loop0 in heartbeat form
    HEARTBEAT_loop0_cloned(startIters, maxIters, constLiveIns, liveInEnvs, 0, 0);

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
void HEARTBEAT_loop0_cloned(uint64_t *startIters, uint64_t *maxIters, uint64_t *constLiveIns, uint64_t **liveInEnvs, uint64_t myLevel, uint64_t startingLevel) {
  // load const live-ins
  int vertices = (int)constLiveIns[1];

  // allocate live-in environment for loop1
  uint64_t liveInEnvLoop1[1 * 8];
  liveInEnvs[(myLevel + 1) * 8] = (uint64_t *)liveInEnvLoop1;

#if defined(CHUNK_LOOP_ITERATIONS)
  int low, high;

  // store into live-in environment for loop1
  liveInEnvLoop1[0 * 8] = (uint64_t)&low;

  for (; ;) {
    low = (int)startIters[myLevel * 8];
    high = (int)std::min(maxIters[myLevel * 8], startIters[myLevel * 8] + CHUNKSIZE_0);
    startIters[myLevel * 8] = (uint64_t)(high - 1);

    for (; low < high; low++) {
      // set the start and max iteation for loop1
      startIters[(myLevel + 1) * 8] = 0;
      maxIters[(myLevel + 1) * 8] = (uint64_t)vertices;

      HEARTBEAT_loop1_cloned(startIters, maxIters, constLiveIns, liveInEnvs, myLevel + 1, startingLevel);
    }

    if (low == (int)maxIters[myLevel]) {
      break;
    }
    loop_handler(startIters, maxIters, constLiveIns, liveInEnvs, myLevel, startingLevel, 2, splittingTasks, leftoverTasks, nullptr);
    if (low == (int)maxIters[myLevel]) {
      break;
    }
    startIters[myLevel * 8] = (uint64_t)low;
  }
#else
  // store into live-in environment for loop1
  liveInEnvLoop1[0 * 8] = (uint64_t)&startIters[myLevel * 8];

  for (; (int)startIters[myLevel * 8] < (int)maxIters[myLevel * 8]; (int)startIters[myLevel * 8]++) {
    // set the start and max iteation for loop1
    startIters[(myLevel + 1) * 8] = 0;
    maxIters[(myLevel + 1) * 8] = (uint64_t)vertices;

    HEARTBEAT_loop1_cloned(startIters, maxIters, constLiveIns, liveInEnvs, myLevel + 1, startingLevel);
    loop_handler(startIters, maxIters, constLiveIns, liveInEnvs, myLevel, startingLevel, 2, splittingTasks, leftoverTasks, nullptr);
  }
#endif
  return;
}

void HEARTBEAT_loop1_cloned(uint64_t *startIters, uint64_t *maxIters, uint64_t *constLiveIns, uint64_t **liveInEnvs, uint64_t myLevel, uint64_t startingLevel) {
  // load const live-ins
  int *dist = (int *)constLiveIns[0];
  int vertices = (int)constLiveIns[1];
  int via = (int)constLiveIns[2];

  // load live-in environment
  uint64_t *liveInEnv = liveInEnvs[myLevel * 8];
  int from = *(int *)liveInEnv[0 * 8];

#if defined(CHUNK_LOOP_ITERATIONS)
  int low, high;
  for (; ;) {
    low = (int)startIters[myLevel * 8];
    high = (int)std::min(maxIters[myLevel * 8], startIters[myLevel * 8] + CHUNKSIZE_1);

    for (; low < high; low++) {
      if ((from != low) && (from != via) && (low != via)) {
        SUB(dist, vertices, from, low) =
          std::min(SUB(dist, vertices, from, low),
                  SUB(dist, vertices, from, via) + SUB(dist, vertices, via, low));
      }
    }

    if (low == (int)maxIters[myLevel * 8]) {
      break;
    }
    startIters[myLevel * 8] = (uint64_t)low;
    loop_handler(startIters, maxIters, constLiveIns, liveInEnvs, myLevel, startingLevel, 2, splittingTasks, leftoverTasks, leafTasks);
  }
#else
  for (; (int)startIters[myLevel * 8] < (int)maxIters[myLevel * 8]; (int)startIters[myLevel * 8]++) {
    if ((from != (int)startIters[myLevel * 8]) && (from != via) && ((int)startIters[myLevel * 8] != via)) {
      SUB(dist, vertices, from, (int)startIters[myLevel * 8]) =
        std::min(SUB(dist, vertices, from, (int)startIters[myLevel * 8]),
                SUB(dist, vertices, from, via) + SUB(dist, vertices, via, (int)startIters[myLevel * 8]));
    }
    loop_handler(startIters, maxIters, constLiveIns, liveInEnvs, myLevel, startingLevel, 2, splittingTasks, leftoverTasks, leafTasks);
  }
#endif

  return;
}

// Leftover loops
void HEARTBEAT_loop1_leftover(uint64_t *startIters, uint64_t *maxIters, uint64_t *constLiveIns, uint64_t **liveInEnvs, uint64_t myLevel, uint64_t startingLevel) {
#ifndef LEFTOVER_SPLITTABLE

  // load const live-ins
  int *dist = (int *)constLiveIns[0];
  int vertices = (int)constLiveIns[1];
  int via = (int)constLiveIns[2];

  // load live-in environment
  uint64_t *liveInEnv = liveInEnvs[myLevel * 8];
  int from = *(int *)liveInEnv[0 * 8];

  for (int to = (int)startIters[myLevel * 8]; to < (int)maxIters[myLevel * 8]; to++) {
    if ((from != to) && (from != via) && (to != via)) {
      SUB(dist, vertices, from, to) =
        std::min(SUB(dist, vertices, from, to),
                SUB(dist, vertices, from, via) + SUB(dist, vertices, via, to));
    }
  }
  startIters[myLevel * 8] = (uint64_t)(int)maxIters[myLevel * 8];

  return;

#else

  HEARTBEAT_loop1_cloned(startIters, maxIters, constLiveIns, liveInEnvs, myLevel, startingLevel);

  return;

#endif
}

// Cloned optimized leaf loops
void HEARTBEAT_loop1_optimized(uint64_t *startIter, uint64_t *maxIter, uint64_t *constLiveIns, uint64_t *liveInEnv) {
  // load const live-ins
  int *dist = (int *)constLiveIns[0];
  int vertices = (int)constLiveIns[1];
  int via = (int)constLiveIns[2];

  // load live-in environment
  int from = *(int *)liveInEnv[0 * 8];

#if defined(CHUNK_LOOP_ITERATIONS)
  int low, high;
  for (; ;) {
    low = (int)startIter[0 * 8];
    high = (int)std::min(maxIter[0 * 8], startIter[0 * 8] + CHUNKSIZE_1);

    for (; low < high; low++) {
      if ((from != low) && (from != via) && (low != via)) {
        SUB(dist, vertices, from, low) =
          std::min(SUB(dist, vertices, from, low),
                  SUB(dist, vertices, from, via) + SUB(dist, vertices, via, low));
      }
    }

    if (low == (int)maxIter[0 * 8]) {
      break;
    }
    startIter[0 * 8] = (uint64_t)low;
    loop_handler_optimized(startIter, maxIter, constLiveIns, liveInEnv, &HEARTBEAT_loop1_optimized);
  }
#else
  for (; (int)startIter[0 * 8] < (int)maxIter[0 * 8]; (int)startIter[0 * 8]++) {
    if ((from != (int)startIter[0 * 8]) && (from != via) && ((int)startIter[0 * 8] != via)) {
      SUB(dist, vertices, from, (int)startIter[0 * 8]) =
        std::min(SUB(dist, vertices, from, (int)startIter[0 * 8]),
                SUB(dist, vertices, from, via) + SUB(dist, vertices, via, (int)startIter[0 * 8]));
    }
    loop_handler_optimized(startIter, maxIter, constLiveIns, liveInEnv, &HEARTBEAT_loop1_optimized);
  }
#endif

  return;
}

}
