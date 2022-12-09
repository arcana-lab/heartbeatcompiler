#pragma once

#include "loop_handler.hpp"

namespace sum_array {

void sum_array_heartbeat_versioning(char *, uint64_t, uint64_t, uint64_t &);
void HEARTBEAT_loop0(char *, uint64_t, uint64_t, uint64_t &);
uint64_t HEARTBEAT_loop0_cloned(uint64_t *, uint64_t *, uint64_t *, uint64_t *, uint64_t *, uint64_t);

static bool run_heartbeat = true;

// Entry function for the benchmark
void sum_array_heartbeat_versioning(char *a, uint64_t low, uint64_t high, uint64_t &result) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate const live-ins
    uint64_t constLiveIns[1];
    constLiveIns[0] = (uint64_t)a;

    // allocate the startIter and maxIter array
    uint64_t startIter[1 * 8];
    uint64_t maxIter[1 * 8];

    // allocate live-in environment for loop0
    uint64_t liveInEnv[1 * 8];

    // allocate live-out environment for loop0
    uint64_t liveOutEnv[1 * 8];

    // allocate reduction array for loop0
    uint64_t redArrLiveOut0[1 * 8];
    liveOutEnv[0 * 8] = (uint64_t)redArrLiveOut0;

    // set the start and max iteration for loop0
    startIter[0 * 8] = low;
    maxIter[0 * 8] = high;

    // invoke loop0 in heartbeat form
    HEARTBEAT_loop0_cloned(startIter, maxIter, constLiveIns, liveInEnv, liveOutEnv, 0);
    result = 0 + redArrLiveOut0[0 * 8];

    run_heartbeat = true;
  } else {
    uint64_t r;
    HEARTBEAT_loop0(a, low, high, r);
    result = 0 + r;
  }
}

// Outlined loops
void HEARTBEAT_loop0(char *a, uint64_t low, uint64_t high, uint64_t &r) {
  for (uint64_t i = low; i < high; i++) {
    r += a[i];
  }

  return;
}

// Cloned loops
uint64_t HEARTBEAT_loop0_cloned(uint64_t *startIter, uint64_t *maxIter, uint64_t *constLiveIns, uint64_t *liveInEnv, uint64_t *liveOutEnv, uint64_t myIndex) {
  // load const live-ins
  char *a = (char *)constLiveIns[0];

  // initialize my private copy of reduction array
  uint64_t *redArrLiveOut0 = (uint64_t *)liveOutEnv[0 * 8];
  redArrLiveOut0[myIndex * 8] = 0;

  // allocate live-out environment for kids
  uint64_t liveOutEnvKids[1 * 8];

  // allocate reduction array for kids
  uint64_t redArrLiveOut0Kids[2 * 8];
  liveOutEnvKids[0 * 8] = (uint64_t)redArrLiveOut0Kids;

  uint64_t rc = LLONG_MAX;
  uint64_t r_private = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  for (; ;) {
    uint64_t low = startIter[0 * 8];
    uint64_t high = std::min(maxIter[0 * 8], startIter[0 * 8] + CHUNKSIZE_0);

    for (; low < high; low++) {
      r_private += a[low];
    }

    if (low == maxIter[0 * 8]) {
      break;
    }
    startIter[0 * 8] = low;
    rc = loop_handler_optimized(startIter, maxIter, constLiveIns, liveInEnv, liveOutEnvKids, &HEARTBEAT_loop0_cloned);
  }
#else
  for (; startIter[0 * 8] < maxIter[0 * 8]; startIter[0 * 8]++) {
    r_private += a[startIter];
    rc = loop_handler_optimized(startIter, maxIter, constLiveIns, liveInEnv, liveOutEnvKids, &HEARTBEAT_loop0_cloned);
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
