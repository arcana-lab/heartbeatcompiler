#pragma once

#include "loop_handler.hpp"

#ifdef CHUNK_LOOP_ITERATIONS
  #ifndef CHUNKSIZE_1
    #define CHUNKSIZE_1 32
  #endif
#endif
#define SUB(array, row_sz, i, j) (array[i * row_sz + j])

/*
 * User defined function to determine the index of the leftover task
 * needs to be defined outside the namespace
 */
uint64_t getLeftoverTaskIndex(uint64_t splittingLevel, uint64_t myLevel) {
  return 0;
}

namespace floyd_warshall {

void floyd_warshall_heartbeat_versioning(int *, int);
void HEARTBEAT_loop0(int *, int, int);
void HEARTBEAT_loop1(int *, int, int, int);
void HEARTBEAT_loop0_cloned(uint64_t *, uint64_t *, uint64_t **, uint64_t);
void HEARTBEAT_loop1_cloned(uint64_t *, uint64_t *, uint64_t **, uint64_t);
void HEARTBEAT_loop1_leftover(uint64_t *, uint64_t *, uint64_t **, uint64_t);

typedef void(*functionPointer)(uint64_t *, uint64_t *, uint64_t **, uint64_t);
functionPointer splittingTasks[2] = {
  &HEARTBEAT_loop0_cloned,
  &HEARTBEAT_loop1_cloned
};
functionPointer leftoverTasks[1] = {
  &HEARTBEAT_loop1_leftover
};

static bool run_heartbeat = true;

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

    // allocate the startIters and maxIters array
    uint64_t startIters[2 * 8];
    uint64_t maxIters[2 * 8];

    // allocate live-in environments
    uint64_t *liveInEnvs[2 * 8];

    // allocate live-in environment for loop0
    uint64_t liveInEnvLoop0[3 * 8];
    liveInEnvs[0 * 8] = (uint64_t *)liveInEnvLoop0;

    // store into live-in environment for loop0
    liveInEnvLoop0[0 * 8] = (uint64_t)dist;
    liveInEnvLoop0[1 * 8] = (uint64_t)vertices;
    liveInEnvLoop0[2 * 8] = (uint64_t)via;

    // set the start and max iteration for loop0
    startIters[0 * 8] = 0;
    maxIters[0 * 8] = vertices;

    // invoke loop0 in heartbeat form
    HEARTBEAT_loop0_cloned(startIters, maxIters, liveInEnvs, 0);

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
void HEARTBEAT_loop0_cloned(uint64_t *startIters, uint64_t *maxIters, uint64_t **liveInEnvs, uint64_t myLevel) {
  // load live-in environment
  uint64_t *liveInEnv = liveInEnvs[myLevel * 8];
  int *dist = (int *)liveInEnv[0 * 8];
  int vertices = (int)liveInEnv[1 * 8];
  int via = (int)liveInEnv[2 * 8];

  for (; startIters[myLevel * 8] < maxIters[myLevel * 8]; startIters[myLevel * 8]++) {
    // allocate live-in environment for loop1
    uint64_t liveInEnvLoop1[4 * 8];
    liveInEnvs[(myLevel + 1) * 8] = (uint64_t *)liveInEnvLoop1;

    // store into loop1's live-in environment
    liveInEnvLoop1[0 * 8] = (uint64_t)dist;
    liveInEnvLoop1[1 * 8] = (uint64_t)vertices;
    liveInEnvLoop1[2 * 8] = (uint64_t)via;
    liveInEnvLoop1[3 * 8] = (uint64_t)startIters[myLevel * 8];  // from

    // set the start and max iteation for loop1
    startIters[(myLevel + 1) * 8] = 0;
    maxIters[(myLevel + 1) * 8] = vertices;

    HEARTBEAT_loop1_cloned(startIters, maxIters, liveInEnvs, myLevel + 1);
#if defined(ENABLE_HEARTBEAT_PROMOTION)
    loop_handler(startIters, maxIters, liveInEnvs, myLevel, splittingTasks, leftoverTasks);
#endif
  }

  return;
}

void HEARTBEAT_loop1_cloned(uint64_t *startIters, uint64_t *maxIters, uint64_t **liveInEnvs, uint64_t myLevel) {
  // load live-in environment
  uint64_t *liveInEnv = liveInEnvs[myLevel * 8];
  int *dist = (int *)liveInEnv[0 * 8];
  int vertices = (int)liveInEnv[1 * 8];
  int via = (int)liveInEnv[2 * 8];
  int from = (int)liveInEnv[3 * 8];

#if defined(CHUNK_LOOP_ITERATIONS)
  for (; ;) {
    uint64_t low = startIters[myLevel * 8];
    uint64_t high = std::min(maxIters[myLevel * 8], startIters[myLevel * 8] + CHUNKSIZE_1);

    for (uint64_t to = low; to < high; to++) {
      if ((from != to) && (from != via) && (to != via)) {
        SUB(dist, vertices, from, to) =
          std::min(SUB(dist, vertices, from, to),
                  SUB(dist, vertices, from, via) + SUB(dist, vertices, via, to));
      }
    }

    startIters[myLevel * 8] = high;
    if (!(startIters[myLevel * 8] < maxIters[myLevel * 8])) {
      break;
    }
#if defined(ENABLE_HEARTBEAT_PROMOTION)
    loop_handler(startIters, maxIters, liveInEnvs, myLevel, splittingTasks, leftoverTasks);
#endif
  }
#else
  for (; startIters[myLevel * 8] < maxIters[myLevel * 8]; startIters[myLevel * 8]++) {
    if ((from != startIters[myLevel * 8]) && (from != via) && (startIters[myLevel * 8] != via)) {
      SUB(dist, vertices, from, startIters[myLevel * 8]) =
        std::min(SUB(dist, vertices, from, startIters[myLevel * 8]),
                SUB(dist, vertices, from, via) + SUB(dist, vertices, via, startIters[myLevel * 8]));
    }
#if defined(ENABLE_HEARTBEAT_PROMOTION)
    loop_handler(startIters, maxIters, liveInEnvs, myLevel, splittingTasks, leftoverTasks);
#endif
  }
#endif

  return;
}

void HEARTBEAT_loop1_leftover(uint64_t *startIters, uint64_t *maxIters, uint64_t **liveInEnvs, uint64_t myLevel) {
#ifndef LEFTOVER_SPLITTABLE

  // load live-in environment
  uint64_t *liveInEnv = liveInEnvs[myLevel * 8];
  int *dist = (int *)liveInEnv[0 * 8];
  int vertices = (int)liveInEnv[1 * 8];
  int via = (int)liveInEnv[2 * 8];
  int from = (int)liveInEnv[3 * 8];

  for (uint64_t to = startIters[myLevel * 8]; to < maxIters[myLevel * 8]; to++) {
    if ((from != to) && (from != via) && (to != via)) {
      SUB(dist, vertices, from, to) =
        std::min(SUB(dist, vertices, from, to),
                SUB(dist, vertices, from, via) + SUB(dist, vertices, via, to));
    }
  }
  startIters[myLevel * 8] = maxIters[myLevel * 8];

  return;

#else

  HEARTBEAT_loop1_cloned(startIters, maxIters, liveInEnvs, myLevel);

  return;

#endif
}

}
