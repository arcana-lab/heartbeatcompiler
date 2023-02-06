#pragma once

#include "loop_handler.hpp"

#define CACHELINE     8
#define LIVE_OUT_ENV  1

#define NUM_LEVELS  1
#define LEVEL_ZERO  0

/*
 * Benchmark-specific variable indicating the level of nested loop
 * needs to be defined outside the namespace
 */
uint64_t numLevels;

void sum_array_heartbeat_versioning(double *, uint64_t, uint64_t, double &);
void HEARTBEAT_loop0(double *, uint64_t, uint64_t, double &);

static bool run_heartbeat = true;
uint64_t *constLiveIns;

// Entry function for the benchmark
void sum_array_heartbeat_versioning(double *a, uint64_t low, uint64_t high, double &result) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // initialize number of levels for this nested loop
    numLevels = NUM_LEVELS;

    // allocate const live-ins
    constLiveIns = (uint64_t *)alloca(sizeof(uint64_t) * 1);
    constLiveIns[0] = (uint64_t)a;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS * CACHELINE];

    // allocate reduction array (as live-out environment) for loop0
    double redArrLiveOut0Loop0[1 * CACHELINE];
    cxts[LEVEL_ZERO * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0Loop0;

    // invoke loop0 in heartbeat form
    HEARTBEAT_loop0_slice(cxts, 0, low, high);
    result += redArrLiveOut0Loop0[0 * 8];

    run_heartbeat = true;
  } else {
    double r = 0.0;
    HEARTBEAT_loop0(a, low, high, r);
    result += r;
  }
}

// Outlined loops
void HEARTBEAT_loop0(double *a, uint64_t low, uint64_t high, double &r) {
  for (uint64_t i = low; i < high; i++) {
    r += a[i];
  }

  return;
}
