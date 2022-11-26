#pragma once

#include "loop_handler.hpp"

#ifdef CHUNK_LOOP_ITERATIONS
  #ifndef CHUNKSIZE_0
    #define CHUNKSIZE_0 64
  #endif
#endif

#if defined(HEARTBEAT_VERSIONING)
/*
 * User defined function to determine the index of the leftover task
 * needs to be defined outside the namespace
 */
uint64_t getLeftoverTaskIndex(uint64_t splittingLevel, uint64_t myLevel) {
  return 0;
}
#endif

namespace sum_array {

#if defined(HEARTBEAT_VERSIONING)

void sum_array_heartbeat_versioning(char *, uint64_t, uint64_t, uint64_t &);
void HEARTBEAT_loop0(char *, uint64_t, uint64_t, uint64_t &);
uint64_t HEARTBEAT_loop0_cloned(uint64_t *, uint64_t *, uint64_t **, uint64_t **, uint64_t, uint64_t);

typedef uint64_t(*functionPointer)(uint64_t *, uint64_t *, uint64_t **, uint64_t **, uint64_t, uint64_t);
functionPointer splittingTasks[1] = {
  &HEARTBEAT_loop0_cloned
};

static bool run_heartbeat = true;

// Entry function for the benchmark
void sum_array_heartbeat_versioning(char *a, uint64_t low, uint64_t high, uint64_t &result) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate the startIters and maxIters array
    uint64_t startIters[1 * 8];
    uint64_t maxIters[1 * 8];

    // allocate live-in and live-out environments
    uint64_t *liveInEnvs[1 * 8];
    uint64_t *liveOutEnvs[1 * 8];

    // allocate live-in environment for loop0
    uint64_t liveInEnvLoop0[1 * 8];
    liveInEnvs[0 * 8] = liveInEnvLoop0;

    // allocate live-out environment for loop0
    uint64_t liveOutEnvLoop0[1 * 8];
    liveOutEnvs[0 * 8] = liveOutEnvLoop0;

    // allocate reduction array for Loop0
    uint64_t redArrLiveOut0Loop0[1 * 8];
    liveOutEnvLoop0[0 * 8] = (uint64_t)redArrLiveOut0Loop0;

    // store into live-in environment
    liveInEnvLoop0[0 * 8] = (uint64_t)a;

    // set the start and max iteration for loop0
    startIters[0 * 8] = low;
    maxIters[0 * 8] = high;

    // invoke loop0 in heartbeat form
    HEARTBEAT_loop0_cloned(startIters, maxIters, liveInEnvs, liveOutEnvs, 0, 0);
    result = 0 + redArrLiveOut0Loop0[0 * 8];

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
uint64_t HEARTBEAT_loop0_cloned(uint64_t *startIters, uint64_t *maxIters, uint64_t **liveInEnvs, uint64_t **liveOutEnvs, uint64_t myLevel, uint64_t myIndex) {
  // load live-in environment
  uint64_t *liveInEnv = liveInEnvs[myLevel * 8];
  char *a = (char *)liveInEnv[0 * 8];

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
  uint64_t r_private = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  for (; ;) {
    uint64_t low = startIters[myLevel * 8];
    uint64_t high = std::min(maxIters[myLevel * 8], startIters[myLevel * 8] + CHUNKSIZE_0);

    for (uint64_t i = low; i < high; i++) {
      r_private += a[i];
    }

    startIters[myLevel * 8] = high;
    if (!(startIters[myLevel * 8] < maxIters[myLevel * 8])) {
      break;
    }
#if defined(ENABLE_HEARTBEAT_PROMOTION)
    rc = loop_handler(startIters, maxIters, liveInEnvs, liveOutEnvs, myLevel, myIndex, splittingTasks, nullptr);
#endif
  }
#else
  for (; startIters[myLevel * 8] < maxIters[myLevel * 8]; startIters[myLevel * 8]++) {
    r_private += a[startIters[myLevel]];
#if defined(ENABLE_HEARTBEAT_PROMOTION)
    rc = loop_handler(startIters, maxIters, liveInEnvs, liveOutEnvs, myLevel, myIndex, splittingTasks, nullptr);
#endif
  }
#endif

  // reduction
  if (rc == LLONG_MAX) {
    redArrLiveOut0[myIndex * 8] += r_private;
  } else if (rc == myLevel) {
    redArrLiveOut0[myIndex * 8] += r_private + redArrLiveOut0Kids[0 * 8] + redArrLiveOut0Kids[1 * 8];
    return LLONG_MAX;
  } else {
    redArrLiveOut0[myIndex * 8] += r_private + redArrLiveOut0Kids[0 * 8] + redArrLiveOut0Kids[1 * 8];
  }

  return rc;
}

#elif defined(HEARTBEAT_VERSIONING_OPTIMIZED)

void sum_array_heartbeat_versioning_optimized(char *, uint64_t, uint64_t, uint64_t &);
void HEARTBEAT_loop0(char *, uint64_t, uint64_t, uint64_t &);
void HEARTBEAT_loop0_cloned(uint64_t *, uint64_t *, uint64_t *, uint64_t *, uint64_t);

// optimization: cloned version of loop itself is the splittingTask

static bool run_heartbeat = true;

// Entry function for the benchmark
void sum_array_heartbeat_versioning_optimized(char *a, uint64_t low, uint64_t high, uint64_t &result) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate the startIter and maxIter array
    uint64_t startIter[1 * 8];
    uint64_t maxIter[1 * 8];

    // allocate live-in and live-out environments
    // optimization: with only level 0, there's only one live-in/out environment

    // allocate live-in environment for loop0
    uint64_t liveInEnv[1 * 8];

    // allocate live-out environment for loop0
    uint64_t liveOutEnv[1 * 8];

    // allocate reduction array for loop0
    uint64_t redArrLiveOut0[1 * 8];
    liveOutEnv[0 * 8] = (uint64_t)redArrLiveOut0;

    // store into live-in environment
    liveInEnv[0 * 8] = (uint64_t)a;

    // set the start and max iteration for loop0
    startIter[0 * 8] = low;
    maxIter[0 * 8] = high;

    // invoke loop0 in heartbeat form
    // optimization, no need to pass myLevel information, it's 0 by default
    HEARTBEAT_loop0_cloned(startIter, maxIter, liveInEnv, liveOutEnv, 0);
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
void HEARTBEAT_loop0_cloned(uint64_t *startIter, uint64_t *maxIter, uint64_t *liveInEnv, uint64_t *liveOutEnv, uint64_t myIndex) {
  // allocate live-out environment for kids
  uint64_t liveOutEnvKids[1 * 8];

  // allocate reduction array for kids
  uint64_t redArrLiveOut0Kids[2 * 8];
  liveOutEnvKids[0 * 8] = (uint64_t)redArrLiveOut0Kids;

  // load live-in environment
  char *a = (char *)liveInEnv[0 * 8];

  // initialize my private copy of reduction array
  uint64_t *redArrLiveOut0 = (uint64_t *)liveOutEnv[0 * 8];
  redArrLiveOut0[myIndex * 8] = 0;

  int rc = 0;
  uint64_t r_private = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  for (; ;) {
    uint64_t low = startIter[0 * 8];
    uint64_t high = std::min(maxIter[0 * 8], startIter[0 * 8] + CHUNKSIZE_0);

    for (; low < high; low++) {
      r_private += a[low];
    }

    startIter[0 * 8] = high;
    if (!(startIter[0 * 8] < maxIter[0 * 8])) {
      break;
    }
#if defined(ENABLE_HEARTBEAT_PROMOTION)
    rc = loop_handler(startIter, maxIter, liveInEnv, liveOutEnvKids, &HEARTBEAT_loop0_cloned);
#endif
  }
#else
  for (; startIter[0 * 8] < maxIter[0 * 8]; startIter[0 * 8]++) {
    r_private += a[startIter];
#if defined(ENABLE_HEARTBEAT_PROMOTION)
    rc = loop_handler(startIter, maxIter, liveInEnv, liveOutEnvKids, &HEARTBEAT_loop0_cloned);
#endif
  }
#endif

  // reduction
  if (rc == 1) {
    redArrLiveOut0[myIndex * 8] += r_private + redArrLiveOut0Kids[0 * 8] + redArrLiveOut0Kids[1 * 8];
  } else {
    redArrLiveOut0[myIndex * 8] += r_private;
  }

  return;
}

#else

  #error "Need to specific the version of heartbeat, e.g., HEARTBEAT_BRANCHES, HEARTBEAT_VERSIONING

#endif

}
