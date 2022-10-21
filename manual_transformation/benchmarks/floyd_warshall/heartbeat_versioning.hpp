#pragma once

#include "loop_handler.hpp"

#define SUB(array, row_sz, i, j) (array[i * row_sz + j])

namespace floyd_warshall {

void floyd_warshall_heartbeat_versioning(int *, int);
void HEARTBEAT_loop0(int *, int, int);
void HEARTBEAT_loop1(int *, int, int, int);
void HEARTBEAT_loop0_cloned(uint64_t *, uint64_t *, void **, uint64_t);
void HEARTBEAT_loop1_cloned(uint64_t *, uint64_t *, void **, uint64_t);
void HEARTBEAT_loop1_leftover(uint64_t *, uint64_t *, void **, uint64_t);

typedef void(*functionPointer)(uint64_t *, uint64_t *, void **, uint64_t);
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

    // allocate live-in environment for loop0
    uint64_t *liveInEnvironments[2];
    uint64_t liveInEnvironmentForLoop0[3];
    liveInEnvironments[0] = (uint64_t *)liveInEnvironmentForLoop0;

    // store into live-in environment
    liveInEnvironmentForLoop0[0] = (uint64_t)dist;
    liveInEnvironmentForLoop0[1] = (uint64_t)vertices;
    liveInEnvironmentForLoop0[2] = (uint64_t)via;

    // allocate the startIterations and maxIterations array
    uint64_t startIterations[2];
    uint64_t maxIterations[2];
    
    // set the start and max iteration for loop0
    startIterations[0] = 0;
    maxIterations[0] = vertices;

    // invoke loop0 in heartbeat form
    HEARTBEAT_loop0_cloned(startIterations, maxIterations, (void **)liveInEnvironments, 0);

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
void HEARTBEAT_loop0_cloned(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel) {
  // load live-in environment
  uint64_t *liveInEnvironment = ((uint64_t **)liveInEnvironments)[myLevel];
  int *dist = (int *)liveInEnvironment[0];
  int vertices = (int)liveInEnvironment[1];
  int via = (int)liveInEnvironment[2];

  // allocate live-in environment for loop1
  uint64_t liveInEnvironmentForLoop1[4];
  ((uint64_t **)liveInEnvironments)[myLevel + 1] = (uint64_t *)liveInEnvironmentForLoop1;

  // store into loop1's live-in environment
  liveInEnvironmentForLoop1[0] = (uint64_t)dist;
  liveInEnvironmentForLoop1[1] = (uint64_t)vertices;
  liveInEnvironmentForLoop1[2] = (uint64_t)via;

  for (; startIterations[myLevel] < maxIterations[myLevel]; startIterations[myLevel]++) {
    loop_handler(startIterations, maxIterations, (void **)liveInEnvironments, myLevel, splittingTasks, leftoverTasks);

    // store into loop1's live-in environment
    liveInEnvironmentForLoop1[3] = (uint64_t)startIterations[myLevel];  // from
    
    // set the start and max iteation for loop1
    startIterations[myLevel + 1] = 0;
    maxIterations[myLevel + 1] = vertices;

    HEARTBEAT_loop1_cloned(startIterations, maxIterations, liveInEnvironments, myLevel + 1);
  }

  return;
}

void HEARTBEAT_loop1_cloned(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel) {
  // load live-in environment
  uint64_t *liveInEnvironment = ((uint64_t **)liveInEnvironments)[myLevel];
  int *dist = (int *)liveInEnvironment[0];
  int vertices = (int)liveInEnvironment[1];
  int via = (int)liveInEnvironment[2];
  int from = (int)liveInEnvironment[3];

  for (; startIterations[myLevel] < maxIterations[myLevel]; startIterations[myLevel]++) {
    loop_handler(startIterations, maxIterations, (void **)liveInEnvironments, myLevel, splittingTasks, leftoverTasks);

    if ((from != startIterations[myLevel]) && (from != via) && (startIterations[myLevel] != via)) {
      SUB(dist, vertices, from, startIterations[myLevel]) =
        std::min(SUB(dist, vertices, from, startIterations[myLevel]),
                SUB(dist, vertices, from, via) + SUB(dist, vertices, via, startIterations[myLevel]));
    }
  }

  return;
}

void HEARTBEAT_loop1_leftover(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel) {
  // load live-in environment
  uint64_t *liveInEnvironment = ((uint64_t **)liveInEnvironments)[myLevel];
  int *dist = (int *)liveInEnvironment[0];
  int vertices = (int)liveInEnvironment[1];
  int via = (int)liveInEnvironment[2];
  int from = (int)liveInEnvironment[3];

  for (; startIterations[myLevel] < maxIterations[myLevel]; startIterations[myLevel]++) {
    if ((from != startIterations[myLevel]) && (from != via) && (startIterations[myLevel] != via)) {
      SUB(dist, vertices, from, startIterations[myLevel]) =
        std::min(SUB(dist, vertices, from, startIterations[myLevel]),
                SUB(dist, vertices, from, via) + SUB(dist, vertices, via, startIterations[myLevel]));
    }
  }

  return;
}

}
