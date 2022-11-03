#pragma once

#include "loop_handler.hpp"

#ifdef CHUNK_LOOP_ITERATIONS
  #ifndef CHUNKSIZE_0
    #define CHUNKSIZE_0 64
  #endif
#endif

namespace sum_array {

#if defined(HEARTBEAT_VERSIONING)

void sum_array_heartbeat_versioning(char *, uint64_t, uint64_t, uint64_t &);
void HEARTBEAT_loop0(char *, uint64_t, uint64_t, uint64_t &);
void HEARTBEAT_loop0_cloned(uint64_t *, uint64_t *, void **, void **, uint64_t, uint64_t);

typedef void(*functionPointer)(uint64_t *, uint64_t *, void **, void **, uint64_t, uint64_t);
functionPointer splittingTasks[1] = {
  &HEARTBEAT_loop0_cloned
};

static bool run_heartbeat = true;

// Entry function for the benchmark
void sum_array_heartbeat_versioning(char *a, uint64_t low, uint64_t high, uint64_t &result) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate the startIterations and maxIterations array
    uint64_t startIterations[1 * 8];
    uint64_t maxIterations[1 * 8];

    // allocate live-in and live-out environments
    uint64_t *liveInEnvironments[1];
    uint64_t *liveOutEnvironments[1];

    // allocate live-in environment for loop0
    uint64_t liveInEnvironmentForLoop0[1];
    liveInEnvironments[0] = (uint64_t *)liveInEnvironmentForLoop0;

    // allocate live-out environment for loop0
    uint64_t liveOutEnvironmentForLoop0[1];
    liveOutEnvironments[0] = (uint64_t *)liveOutEnvironmentForLoop0;

    // allocate reduction array for loop1
    uint64_t reductionArrayLiveOut1Loop0[1 * 8];
    liveOutEnvironmentForLoop0[0] = (uint64_t)reductionArrayLiveOut1Loop0;

    // store into live-in environment
    liveInEnvironmentForLoop0[0] = (uint64_t)a;

    // set the start and max iteration for loop0
    startIterations[0 * 8] = low;
    maxIterations[0 * 8] = high;

    // invoke loop0 in heartbeat form
    HEARTBEAT_loop0_cloned(startIterations, maxIterations, (void **)liveInEnvironments, (void **)liveOutEnvironments, 0, 0);
    result = 0 + reductionArrayLiveOut1Loop0[0 * 8];

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
void HEARTBEAT_loop0_cloned(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, void **liveOutEnvironments, uint64_t myLevel, uint64_t myIndex) {
  // allocate live-out environment for next level
  uint64_t liveOutEnvironmentNextLevel[1];

  // allocate reduction array for next level
  uint64_t reductionArrayLiveOut1NextLevel[2 * 8];
  liveOutEnvironmentNextLevel[0] = (uint64_t)reductionArrayLiveOut1NextLevel;

  // load live-in environment
  uint64_t *liveInEnvironment = ((uint64_t **)liveInEnvironments)[myLevel];
  char *a = (char *)liveInEnvironment[0];

  // initialize my private copy of reduction array
  uint64_t *liveOutEnvironment = ((uint64_t **)liveOutEnvironments)[myLevel];
  uint64_t *reductionArrayLiveOut1 = (uint64_t *)liveOutEnvironment[0];
  reductionArrayLiveOut1[myIndex * 8] = 0;

  int rc = 0;
  uint64_t r_private = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  for (; ;) {
    rc = loop_handler(startIterations, maxIterations, liveInEnvironments, liveOutEnvironments, (void *)liveOutEnvironmentNextLevel, myLevel, myIndex, splittingTasks, nullptr);
    uint64_t low = startIterations[myLevel * 8];
    uint64_t high = std::min(maxIterations[myLevel * 8], startIterations[myLevel * 8] + CHUNKSIZE_0);

    for (uint64_t i = low; i < high; i++) {
      r_private += a[i];
    }

    startIterations[myLevel * 8] = high;
    if (!(startIterations[myLevel * 8] < maxIterations[myLevel * 8])) {
      break;
    }
  }
#else
  for (; startIterations[myLevel] < maxIterations[myLevel]; startIterations[myLevel]++) {
    rc = loop_handler(startIterations, maxIterations, liveInEnvironments, liveOutEnvironments, (void *)liveOutEnvironmentNextLevel, myLevel, myIndex, splittingTasks, nullptr);
    r_private += a[startIterations[myLevel]];
  }
#endif

  // reduction
  if (rc == 1) {
    reductionArrayLiveOut1[myIndex * 8] += r_private + reductionArrayLiveOut1NextLevel[0 * 8] + reductionArrayLiveOut1NextLevel[1 * 8];
  } else {
    reductionArrayLiveOut1[myIndex * 8] += r_private;
  }

  return;
}

#elif defined(HEARTBEAT_VERSIONING_OPTIMIZED)

void sum_array_heartbeat_versioning(char *, uint64_t, uint64_t, uint64_t &);
void HEARTBEAT_loop0(char *, uint64_t, uint64_t, uint64_t &);
void HEARTBEAT_loop0_cloned(uint64_t, uint64_t, void *, void *, uint64_t);

// optimization: cloned version of loop itself is the splittingTask

static bool run_heartbeat = true;

// Entry function for the benchmark
void sum_array_heartbeat_versioning(char *a, uint64_t low, uint64_t high, uint64_t &result) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate the startIterations and maxIterations array
    // optimization: with only level 0, iteration numbers will be pass around through registers

    // allocate live-in and live-out environments
    // optimization: with only level 0, there's only one live-in/out environment

    // allocate live-in environment for loop0
    uint64_t liveInEnvironment[1];

    // allocate live-out environment for loop0
    uint64_t liveOutEnvironment[1];

    // allocate reduction array for loop1
    uint64_t reductionArrayLiveOut1[1 * 8];
    liveOutEnvironment[0] = (uint64_t)reductionArrayLiveOut1;

    // store into live-in environment
    liveInEnvironment[0] = (uint64_t)a;

    // set the start and max iteration for loop0
    uint64_t startIteration = low;
    uint64_t maxIteration = high;

    // invoke loop0 in heartbeat form
    // optimization, no need to pass myLevel information, it's 0 by default
    HEARTBEAT_loop0_cloned(startIteration, maxIteration, (void *)liveInEnvironment, (void *)liveOutEnvironment, 0);
    result = 0 + reductionArrayLiveOut1[0 * 8];

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
void HEARTBEAT_loop0_cloned(uint64_t startIteration, uint64_t maxIteration, void *liveInEnvironment, void *liveOutEnvironment, uint64_t myIndex) {
  // allocate live-out environment for next level
  uint64_t liveOutEnvironmentNextLevel[1];

  // allocate reduction array for next level
  uint64_t reductionArrayLiveOut1NextLevel[2 * 8];
  liveOutEnvironmentNextLevel[0] = (uint64_t)reductionArrayLiveOut1NextLevel;

  // load live-in environment
  char *a = (char *)((uint64_t *)liveInEnvironment)[0];

  // initialize my private copy of reduction array
  uint64_t *reductionArrayLiveOut1 = (uint64_t *)((uint64_t *)liveOutEnvironment)[0];
  reductionArrayLiveOut1[myIndex * 8] = 0;

  int rc = 0;
  uint64_t r_private = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  for (; ;) {
    rc = loop_handler(startIteration, maxIteration, liveInEnvironment, (void *)liveOutEnvironmentNextLevel, &HEARTBEAT_loop0_cloned);
    uint64_t low = startIteration;
    uint64_t high = std::min(maxIteration, startIteration + CHUNKSIZE_0);

    for (; low < high; low++) {
      r_private += a[low];
    }

    startIteration = high;
    if (!(startIteration < maxIteration)) {
      break;
    }
  }
#else
  for (; startIteration < maxIteration; startIteration++) {
    rc = loop_handler(startIteration, maxIteration, liveInEnvironment, (void *)liveOutEnvironmentNextLevel, &HEARTBEAT_loop0_cloned);
    r_private += a[startIteration];
  }
#endif

  // reduction
  if (rc == 1) {
    reductionArrayLiveOut1[myIndex * 8] += r_private + reductionArrayLiveOut1NextLevel[0 * 8] + reductionArrayLiveOut1NextLevel[1 * 8];
  } else {
    reductionArrayLiveOut1[myIndex * 8] += r_private;
  }

  return;
}

#else

  #error "Need to specific the version of heartbeat, e.g., HEARTBEAT_BRANCHES, HEARTBEAT_VERSIONING

#endif

}