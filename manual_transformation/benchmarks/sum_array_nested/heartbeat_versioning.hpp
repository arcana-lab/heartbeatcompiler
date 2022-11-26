#pragma once

#include "loop_handler.hpp"

#ifdef CHUNK_LOOP_ITERATIONS
  #ifndef CHUNKSIZE_1
    #define CHUNKSIZE_1 64
  #endif
#endif

/*
 * User defined function to determine the index of the leftover task
 * needs to be defined outside the namespace
 */
uint64_t getLeftoverTaskIndex(uint64_t splittingLevel, uint64_t myLevel) {
  return 0;
}

namespace sum_array_nested {

void sum_array_nested_heartbeat_versioning(char **, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t &);
void HEARTBEAT_loop0(char **, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t &);
void HEARTBEAT_loop1(char **, uint64_t, uint64_t, uint64_t, uint64_t &);
uint64_t HEARTBEAT_loop0_cloned(uint64_t *, uint64_t *, uint64_t **, uint64_t **, uint64_t, uint64_t);
uint64_t HEARTBEAT_loop1_cloned(uint64_t *, uint64_t *, uint64_t **, uint64_t **, uint64_t, uint64_t);
uint64_t HEARTBEAT_loop1_leftover(uint64_t *, uint64_t *, uint64_t **, uint64_t **, uint64_t, uint64_t);

typedef uint64_t(*functionPointer)(uint64_t *, uint64_t *, uint64_t **, uint64_t **, uint64_t, uint64_t);
functionPointer splittingTasks[2] = {
  &HEARTBEAT_loop0_cloned,
  &HEARTBEAT_loop1_cloned
};

functionPointer leftoverTasks[1] = {
  &HEARTBEAT_loop1_leftover
};

static bool run_heartbeat = true;

// Entry function for the benchmark
void sum_array_nested_heartbeat_versioning(char **a, uint64_t low1, uint64_t high1, uint64_t low2, uint64_t high2, uint64_t &result) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate the startIters and maxIters array
    uint64_t startIters[2 * 8];
    uint64_t maxIters[2 * 8];

    // allocate live-in and live-out environments
    uint64_t *liveInEnvs[2 * 8];
    uint64_t *liveOutEnvs[2 * 8];

    // allocate live-in environment for loop0
    uint64_t liveInEnvLoop0[3 * 8];
    liveInEnvs[0 * 8] = liveInEnvLoop0;

    // allocate live-out environment for loop0
    uint64_t liveOutEnvLoop0[1 * 8];
    liveOutEnvs[0 * 8] = liveOutEnvLoop0;

    // allocate reduction array for loop0
    uint64_t redArrLiveOut0Loop0[1 * 8];
    liveOutEnvLoop0[0 * 8] = (uint64_t)redArrLiveOut0Loop0;

    // store into live-in environment for loop0
    liveInEnvLoop0[0 * 8] = (uint64_t)a;
    liveInEnvLoop0[1 * 8] = (uint64_t)low2;
    liveInEnvLoop0[2 * 8] = (uint64_t)high2;

    // set the start and max iteration for loop0
    startIters[0 * 8] = low1;
    maxIters[0 * 8] = high1;

    // invoke loop0 in heartbeat form
    uint64_t r0 = 0;
    HEARTBEAT_loop0_cloned(startIters, maxIters, liveInEnvs, liveOutEnvs, 0, 0);
    result += r0 + redArrLiveOut0Loop0[0 * 8];

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
uint64_t HEARTBEAT_loop0_cloned(uint64_t *startIters, uint64_t *maxIters, uint64_t **liveInEnvs, uint64_t **liveOutEnvs, uint64_t myLevel, uint64_t myIndex) {
  // load live-in environment
  uint64_t *liveInEnv = liveInEnvs[myLevel * 8];
  char **a = (char **)liveInEnv[0 * 8];
  uint64_t low2 = (uint64_t)liveInEnv[1 * 8];
  uint64_t high2 = (uint64_t)liveInEnv[2 * 8];

  // initialize my private copy of reduction array
  uint64_t *liveOutEnv = liveOutEnvs[myLevel * 8];
  uint64_t *redArrLiveOut0 = (uint64_t *)liveOutEnv[0 * 8];
  redArrLiveOut0[myIndex * 8] = 0;

  // allocate live-out environment for kids
  uint64_t liveOutEnvKids[1 * 8];
  liveOutEnvs[myLevel * 8] = liveOutEnvKids;

  // allocate reduction array for kids
  uint64_t redArrLiveOut0Kids[2 * 8];
  liveOutEnvKids[0 * 8] = (uint64_t)redArrLiveOut0Kids;

  uint64_t rc = LLONG_MAX;
  uint64_t r0_private = 0;
  for (; startIters[myLevel * 8] < maxIters[myLevel * 8]; startIters[myLevel * 8]++) {
    // allocate live-in environment for loop1
    uint64_t liveInEnvLoop1[2 * 8];
    liveInEnvs[(myLevel + 1) * 8] = liveInEnvLoop1;

    // allocate live-out environment for loop1
    uint64_t liveOutEnvLoop1[1 * 8];
    liveOutEnvs[(myLevel + 1) * 8] = liveOutEnvLoop1;

    // allocate reduction array for loop1
    uint64_t redArrLiveOut0Loop1[1 * 8];
    liveOutEnvLoop1[0 * 8] = (uint64_t)redArrLiveOut0Loop1;

    // store into live-in environment for loop1
    liveInEnvLoop1[0 * 8] = (uint64_t)a;
    liveInEnvLoop1[1 * 8] = (uint64_t)startIters[myLevel * 8];

    // set the start and max iteration for loop1
    startIters[(myLevel + 1) * 8] = low2;
    maxIters[(myLevel + 1) * 8] = high2;

    uint64_t r1 = 0;
    rc = std::min(rc, HEARTBEAT_loop1_cloned(startIters, maxIters, liveInEnvs, liveOutEnvs, myLevel + 1, 0));
    r0_private += r1 + redArrLiveOut0Loop1[0 * 8];

#if defined(ENABLE_HEARTBEAT_VERSIONING)
    rc = loop_handler(startIters, maxIters, liveInEnvs, liveOutEnvs, myLevel, myIndex, splittingTasks, leftoverTasks);
#endif
  }

  // reduction
  if (rc == LLONG_MAX) {      // either no heartbeat promotion happens or promotion happens at a higher nested level
    redArrLiveOut0[myIndex * 8] += r0_private;
  } else if (rc == myLevel) { // heartbeat promotion happens at my level
    redArrLiveOut0[myIndex * 8] += r0_private + redArrLiveOut0Kids[0 * 8] + redArrLiveOut0Kids[1 * 8];
    return LLONG_MAX;
  } else {                    // heartbeat promotion happens and splitting happens at a lower nested level
    redArrLiveOut0[myIndex * 8] += r0_private + redArrLiveOut0Kids[0 * 8];
  }

  return rc;
}

uint64_t HEARTBEAT_loop1_cloned(uint64_t *startIters, uint64_t *maxIters, uint64_t **liveInEnvs, uint64_t **liveOutEnvs, uint64_t myLevel, uint64_t myIndex) {
  // load live-in environment
  uint64_t *liveInEnv = liveInEnvs[myLevel * 8];
  char **a = (char **)liveInEnv[0 * 8];
  uint64_t i = (uint64_t)liveInEnv[1 * 8];

  // initialize my private copy of reduction array
  uint64_t *liveOutEnv = liveOutEnvs[myLevel * 8];
  uint64_t *redArrLiveOut0 = (uint64_t *)liveOutEnv[0 * 8];
  redArrLiveOut0[myIndex * 8] = 0;

  // allocate live-out environment for kids
  uint64_t liveOutEnvKids[1 * 8];
  liveOutEnvs[myLevel * 8] = liveOutEnvKids;

  // allocate reduction array for kids
  uint64_t redArrLiveOut0Kids[2 * 8];
  liveOutEnvKids[0 * 8] = (uint64_t)redArrLiveOut0Kids;

  uint64_t rc = LLONG_MAX;
  uint64_t r1_private = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  for (; ;) {
    uint64_t low = startIters[myLevel * 8];
    uint64_t high = std::min(maxIters[myLevel * 8], startIters[myLevel * 8] + CHUNKSIZE_1);

    for (uint64_t j = low; j < high; j++) {
      r1_private += a[i][j];
    }

    startIters[myLevel * 8] = high;
    if (!(startIters[myLevel * 8] < maxIters[myLevel * 8])) {
      break;
    }
#if defined(ENABLE_HEARTBEAT_PROMOTION)
    rc = loop_handler(startIters, maxIters, liveInEnvs, liveOutEnvs, myLevel, myIndex, splittingTasks, leftoverTasks);
#endif
  }
#else
  for (; startIters[myLevel * 8] < maxIters[myLevel * 8]; startIters[myLevel * 8]++) {
    r1_private += a[i][startIters[myLevel * 8]];
#if defined(ENABLE_HEARTBEAT_PROMOTION)
    rc = loop_handler(startIters, maxIters, liveInEnvs, liveOutEnvs, myLevel, myIndex, splittingTasks, leftoverTasks);
#endif
  }
#endif

  // reduction
  if (rc == LLONG_MAX) {      // either no heartbeat promotion happens or promotion happens at a higher nested level
    redArrLiveOut0[myIndex * 8] += r1_private;
  } else if (rc == myLevel) { // heartbeat promotion happens at my level
    redArrLiveOut0[myIndex * 8] += r1_private + redArrLiveOut0Kids[0 * 8] + redArrLiveOut0Kids[1 * 8];
    return LLONG_MAX;
  } else {                    // heartbeat promotion happens and splitting happens at a lower nested level
    redArrLiveOut0[myIndex * 8] += r1_private + redArrLiveOut0Kids[0 * 8];
  }

  return rc;
}

uint64_t HEARTBEAT_loop1_leftover(uint64_t *startIters, uint64_t *maxIters, uint64_t **liveInEnvs, uint64_t **liveOutEnvs, uint64_t myLevel, uint64_t myIndex) {
#ifndef LEFTOVER_SPLITTABLE

  // load live-in environment
  uint64_t *liveInEnv = liveInEnvs[myLevel * 8];
  char **a = (char **)liveInEnv[0 * 8];
  uint64_t i = (uint64_t)liveInEnv[1 * 8];

  // initialize my private copy of reduction array
  uint64_t *liveOutEnv = liveOutEnvs[myLevel * 8];
  uint64_t *redArrLiveOut0 = (uint64_t *)liveOutEnv[0 * 8];
  redArrLiveOut0[0 * 8] = 0;

  uint64_t r1_private = 0;
  for (uint64_t j = startIters[myLevel * 8]; j < maxIters[myLevel * 8]; j++) {
    r1_private += a[i][j];
  }
  startIters[myLevel * 8] = maxIters[myLevel * 8];

  // reduction
  redArrLiveOut0[myIndex * 8] += r1_private;

  return 0;

#else

  return HEARTBEAT_loop1_cloned(startIters, maxIters, liveInEnvs, liveOutEnvs, myLevel, myIndex);

#endif
}

}
