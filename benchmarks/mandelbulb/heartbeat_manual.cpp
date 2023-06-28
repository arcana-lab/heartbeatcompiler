#include "heartbeat_manual.hpp"
#include "loop_handler.hpp"
#include <cstdint>
#include <cmath>

#define unlikely(expr) __builtin_expect(!!(expr), 0)
#define NUM_LEVELS_NEST0 3
#define LEVEL_ZERO 0
#define LEVEL_ONE 1
#define LEVEL_TWO 2
#define CACHELINE 8
#define START_ITER 0
#define MAX_ITER 1
#define LIVE_IN_ENV 2

namespace mandelbulb {

void HEARTBEAT_nest0_loop0(double x0, double y0, double z0, int nx, int ny, int nz, int iterations, double power, double xstep, double ystep, double zstep, unsigned char *output);
void HEARTBEAT_nest0_loop1(double x0, double y0, double z0, int ny, int nz, int iterations, double power, double xstep, double ystep, double zstep, unsigned char *output, int i);
void HEARTBEAT_nest0_loop2(double x0, double y0, double z0, int ny, int nz, int iterations, double power, double xstep, double ystep, double zstep, unsigned char *output, int i, int j);

int64_t HEARTBEAT_nest0_loop0_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem);
int64_t HEARTBEAT_nest0_loop1_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem);
int64_t HEARTBEAT_nest0_loop2_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem);
typedef int64_t (*sliceTasksPointer)(uint64_t *, uint64_t *, uint64_t, task_memory_t *);
sliceTasksPointer slice_tasks_nest0[3] = {
  &HEARTBEAT_nest0_loop0_slice,
  &HEARTBEAT_nest0_loop1_slice,
  &HEARTBEAT_nest0_loop2_slice
};

void HEARTBEAT_nest0_loop_1_0_leftover(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem);
void HEARTBEAT_nest0_loop_2_1_leftover(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem);
void HEARTBEAT_nest0_loop_2_0_leftover(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem);
typedef void (*leftoverTasksPointer)(uint64_t *, uint64_t *, uint64_t, task_memory_t *);
leftoverTasksPointer leftover_tasks_nest0[3] = {
  &HEARTBEAT_nest0_loop_2_0_leftover,
  &HEARTBEAT_nest0_loop_2_1_leftover,
  &HEARTBEAT_nest0_loop_1_0_leftover
};

uint64_t leftover_selector_nest0(uint64_t receivingLevel, uint64_t splittingLevel) {
  if (receivingLevel == 2 && splittingLevel == 0) {
    return 0;
  } else if (receivingLevel == 2 && splittingLevel == 1) {
    return 1;
  } else if (receivingLevel == 1 && splittingLevel == 0) {
    return 2;
  }
}

bool run_heartbeat = true;

// Outlined loops
void HEARTBEAT_nest0_loop0(double x0, double y0, double z0, int nx, int ny, int nz, int iterations, double power, double xstep, double ystep, double zstep, unsigned char *output) {
  if (run_heartbeat) {
    run_heartbeat = false;
    // allocate const live-ins
    uint64_t constLiveIns[11];
    constLiveIns[0] = (uint64_t)&x0;
    constLiveIns[1] = (uint64_t)&y0;
    constLiveIns[2] = (uint64_t)&z0;
    constLiveIns[3] = (uint64_t)ny;
    constLiveIns[4] = (uint64_t)nz;
    constLiveIns[5] = (uint64_t)iterations;
    constLiveIns[6] = (uint64_t)&power;
    constLiveIns[7] = (uint64_t)&xstep;
    constLiveIns[8] = (uint64_t)&ystep;
    constLiveIns[9] = (uint64_t)&zstep;
    constLiveIns[10] = (uint64_t)output;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS_NEST0 * CACHELINE];

    // set start/max iterations for loop0
    cxts[LEVEL_ZERO * CACHELINE + START_ITER] = (uint64_t)0;
    cxts[LEVEL_ZERO * CACHELINE + MAX_ITER] = (uint64_t)nx;

    // allocate the task memory struct and initialize
    task_memory_t tmem;
    heartbeat_start(&tmem);

    // invoke nest0_loop0 in heartbeat form
    HEARTBEAT_nest0_loop0_slice(cxts, constLiveIns, 0, &tmem);

    run_heartbeat = true;
  } else {
    for (int i = 0; i < nx; ++i) {
      HEARTBEAT_nest0_loop1(x0, y0, z0, ny, nz, iterations, power, xstep, ystep, zstep, output, i);
    }
  }
}

void HEARTBEAT_nest0_loop1(double x0, double y0, double z0, int ny, int nz, int iterations, double power, double xstep, double ystep, double zstep, unsigned char *output, int i) {
  for(int j = 0; j < ny; ++j) {
    HEARTBEAT_nest0_loop2(x0, y0, z0, ny, nz, iterations, power, xstep, ystep, zstep, output, i, j);
  }
}

void HEARTBEAT_nest0_loop2(double x0, double y0, double z0, int ny, int nz, int iterations, double power, double xstep, double ystep, double zstep, unsigned char *output, int i, int j) {
  for (int k = 0; k < nz; ++k) {
    double x = x0 + i * xstep;
    double y = y0 + i * ystep;
    double z = z0 + i * zstep;
    uint64_t iter = 0;
    for (; iter < iterations; iter++) {
      auto r = sqrt(x*x + y*y + z*z);
      auto theta = acos(z / r);
      auto phi = atan(y / x);
      x = pow(r, power) * sin(power * theta) * cos(power * phi) + x;
      y = pow(r, power) * sin(power * theta) * sin(power * phi) + y;
      z = pow(r, power) * cos(power * theta) + z;
      if (sqrt(sqrt(x)+sqrt(y)+sqrt(z)) > 1.0) {
        break;
      }
    }
    output[(i*ny + j) * nz + k] = static_cast<unsigned char>(static_cast<double>(iter) / iterations * 255);
  }
}

// Transformed loops
int64_t HEARTBEAT_nest0_loop0_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem) {
  // load start/max iterations
  uint64_t startIter = cxts[LEVEL_ZERO * CACHELINE + START_ITER];
  uint64_t maxIter = cxts[LEVEL_ZERO * CACHELINE + MAX_ITER];

  // load const live-ins
  int ny = (int)constLiveIns[3];

  int64_t rc = 0;
  // store &live-in as live-in environment for loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&startIter;

  for (; startIter < maxIter; startIter++) {
    // store current iteration for loop0
    cxts[LEVEL_ZERO * CACHELINE + START_ITER] = startIter;
    // set start/max iterations for loop1
    cxts[LEVEL_ONE * CACHELINE + START_ITER] = (uint64_t)0;
    cxts[LEVEL_ONE * CACHELINE + MAX_ITER] = (uint64_t)ny;
    rc = HEARTBEAT_nest0_loop1_slice(cxts, constLiveIns, 0, tmem);
    if (rc > 0) {
      // update the exit condition here because there might
      // be tail work to finish
      maxIter = startIter + 1;
    }

    // check the status of rc because, might not need
    // to call the loop_handler in the process of returnning up
    if (rc > 0) {
      break;
    }

#if defined(CHUNK_LOOP_ITERATIONS)
    // don't poll if we haven't finished at least one chunk
    if (has_remaining_chunksize(tmem)) {
      continue;
    }
#endif

#if !defined(ENABLE_ROLLFORWARD)
    if (unlikely(heartbeat_polling(tmem))) {
      cxts[LEVEL_ZERO * CACHELINE + START_ITER] = startIter;
      rc = loop_handler(
        cxts, constLiveIns, LEVEL_ZERO, NUM_LEVELS_NEST0, tmem,
        slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
      );
      if (rc > 0) {
        break;
      }
    }
#else
    cxts[LEVEL_ZERO * CACHELINE + START_ITER] = startIter;
    __rf_handle_wrapper(
      rc, cxts, constLiveIns, LEVEL_ZERO, NUM_LEVELS_NEST0, tmem,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
    );
    if (rc > 0) {
      break;
    }
#endif
  }

  return rc - 1;
}

int64_t HEARTBEAT_nest0_loop1_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem) {
  // load start/max iterations
  uint64_t startIter = cxts[LEVEL_ONE * CACHELINE + START_ITER];
  uint64_t maxIter = cxts[LEVEL_ONE * CACHELINE + MAX_ITER];

  // load const live-ins
  int nz = (int)constLiveIns[4];

  // load live-ins
  int i = (int)*(uint64_t *)cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV];

  // allocate live-in environment for loop2
  uint64_t liveInEnv[2 * CACHELINE];
  cxts[LEVEL_TWO * CACHELINE + LIVE_IN_ENV] = (uint64_t)liveInEnv;
  liveInEnv[0 * CACHELINE] = (uint64_t)i;

  int64_t rc = 0;
  // store live-in for loop2
  liveInEnv[1 * CACHELINE] = (uint64_t)&startIter;

  for (; startIter < maxIter; startIter++) {
    // store current iteration for loop1
    cxts[LEVEL_ONE * CACHELINE + START_ITER] = startIter;
    // set start/max iterations for loop2
    cxts[LEVEL_TWO * CACHELINE + START_ITER] = (uint64_t)0;
    cxts[LEVEL_TWO * CACHELINE + MAX_ITER] = (uint64_t)nz;
    rc = HEARTBEAT_nest0_loop2_slice(cxts, constLiveIns, 0, tmem);
    if (rc > 0) {
      // update the exit condition here because there might
      // be tail work to finish
      maxIter = startIter + 1;
    }

    // check the status of rc because, might not need
    // to call the loop_handler in the process of returnning up
    if (rc > 0) {
      break;
    }

#if defined(CHUNK_LOOP_ITERATIONS)
    // don't poll if we haven't finished at least one chunk
    if (has_remaining_chunksize(tmem)) {
      continue;
    }
#endif

#if !defined(ENABLE_ROLLFORWARD)
    if (unlikely(heartbeat_polling(tmem))) {
      cxts[LEVEL_ONE * CACHELINE + START_ITER] = startIter;
      rc = loop_handler(
        cxts, constLiveIns, LEVEL_ONE, NUM_LEVELS_NEST0, tmem,
        slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
      );
      if (rc > 0) {
        break;
      }
    }
#else
    cxts[LEVEL_ONE * CACHELINE + START_ITER] = startIter;
    __rf_handle_wrapper(
      rc, cxts, constLiveIns, LEVEL_ONE, NUM_LEVELS_NEST0, tmem,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
    );
    if (rc > 0) {
      break;
    }
#endif
  }

  return rc - 1;
}

int64_t HEARTBEAT_nest0_loop2_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem) {
  // load start/max iterations
  uint64_t startIter = cxts[LEVEL_TWO * CACHELINE + START_ITER];
  uint64_t maxIter = cxts[LEVEL_TWO * CACHELINE + MAX_ITER];

  // load const live-ins
  double x0 = *(double *)constLiveIns[0];
  double y0 = *(double *)constLiveIns[1];
  double z0 = *(double *)constLiveIns[2];
  int ny = (int)constLiveIns[3];
  int nz = (int)constLiveIns[4];
  int iterations =(int)constLiveIns[5];
  double power = *(double *)constLiveIns[6];
  double xstep = *(double *)constLiveIns[7];
  double ystep = *(double *)constLiveIns[8];
  double zstep = *(double *)constLiveIns[9];
  unsigned char *output = (unsigned char *)constLiveIns[10];

  // load live-ins
  uint64_t *liveInEnv = (uint64_t *)cxts[LEVEL_TWO * CACHELINE + LIVE_IN_ENV];
  int i = (int)liveInEnv[0 * CACHELINE];
  int j = (int)*(uint64_t *)liveInEnv[1 * CACHELINE];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t chunksize = get_chunksize(tmem);
  for (; startIter < maxIter; startIter += chunksize) {
    uint64_t low = startIter;
    uint64_t high = maxIter < startIter + chunksize ? maxIter : startIter + chunksize;
    for (; low < high; low++) {
      double x = x0 + i * xstep;
      double y = y0 + i * ystep;
      double z = z0 + i * zstep;
      uint64_t iter = 0;
      for (; iter < iterations; iter++) {
        auto r = sqrt(x*x + y*y + z*z);
        auto theta = acos(z / r);
        auto phi = atan(y / x);
        x = pow(r, power) * sin(power * theta) * cos(power * phi) + x;
        y = pow(r, power) * sin(power * theta) * sin(power * phi) + y;
        z = pow(r, power) * cos(power * theta) + z;
        if (sqrt(sqrt(x)+sqrt(y)+sqrt(z)) > 1.0) {
          break;
        }
      }
      output[(i*ny + j) * nz + low] = static_cast<unsigned char>(static_cast<double>(iter) / iterations * 255);
    }

    chunksize = update_remaining_chunksize(tmem, high - startIter, chunksize);
    if (has_remaining_chunksize(tmem)) {
      break;
    }

#if !defined(ENABLE_ROLLFORWARD)
    if (unlikely(heartbeat_polling(tmem))) {
      cxts[LEVEL_TWO * CACHELINE + START_ITER] = low - 1;
      rc = loop_handler(
        cxts, constLiveIns, LEVEL_TWO, NUM_LEVELS_NEST0, tmem,
        slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
      );
      if (rc > 0) {
        break;
      }
    }
#else
    cxts[LEVEL_TWO * CACHELINE + START_ITER] = low - 1;
    __rf_handle_wrapper(
      rc, cxts, constLiveIns, LEVEL_TWO, NUM_LEVELS_NEST0, tmem,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
    );
    if (rc > 0) {
      break;
    }
#endif
  }
#else
  for(; startIter < maxIter; startIter++) {
    double x = x0 + i * xstep;
    double y = y0 + i * ystep;
    double z = z0 + i * zstep;
    uint64_t iter = 0;
    for (; iter < iterations; iter++) {
      auto r = sqrt(x*x + y*y + z*z);
      auto theta = acos(z / r);
      auto phi = atan(y / x);
      x = pow(r, power) * sin(power * theta) * cos(power * phi) + x;
      y = pow(r, power) * sin(power * theta) * sin(power * phi) + y;
      z = pow(r, power) * cos(power * theta) + z;
      if (sqrt(sqrt(x)+sqrt(y)+sqrt(z)) > 1.0) {
        break;
      }
    }
    output[(i*ny + j) * nz + startIter] = static_cast<unsigned char>(static_cast<double>(iter) / iterations * 255);

#if !defined(ENABLE_ROLLFORWARD)
    if (unlikely(heartbeat_polling(tmem))) {
      cxts[LEVEL_TWO * CACHELINE + START_ITER] = startIter;
      rc = loop_handler(
        cxts, constLiveIns, LEVEL_TWO, NUM_LEVELS_NEST0, tmem,
        slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
      );
      if (rc > 0) {
        break;
      }
    }
#else
    cxts[LEVEL_TWO * CACHELINE + START_ITER] = startIter;
    __rf_handle_wrapper(
      rc, cxts, constLiveIns, LEVEL_TWO, NUM_LEVELS_NEST0, tmem,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
    );
    if (rc > 0) {
      break;
    }
#endif
  }
#endif

  return rc - 1;
}

// Leftover tasks
void HEARTBEAT_nest0_loop_2_0_leftover(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem) {
  int64_t rc = 0;
  rc = HEARTBEAT_nest0_loop2_slice(cxts, constLiveIns, myIndex, tmem);
  if (rc > 0) {
    return;
  }

  cxts[LEVEL_ONE * CACHELINE + START_ITER]++;
  rc = HEARTBEAT_nest0_loop1_slice(cxts, constLiveIns, myIndex, tmem);
  if (rc > 0) {
    return;
  }

  cxts[LEVEL_ZERO * CACHELINE + START_ITER]++;
  rc = HEARTBEAT_nest0_loop0_slice(cxts, constLiveIns, myIndex, tmem);
  if (rc > 0) {
    return;
  }

  return;
}

void HEARTBEAT_nest0_loop_2_1_leftover(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem) {
  int64_t rc = 0;
  rc = HEARTBEAT_nest0_loop2_slice(cxts, constLiveIns, myIndex, tmem);
  if (rc > 0) {
    return;
  }

  cxts[LEVEL_ONE * CACHELINE + START_ITER]++;
  rc = HEARTBEAT_nest0_loop1_slice(cxts, constLiveIns, myIndex, tmem);
  if (rc > 0) {
    return;
  }

  return;
}

void HEARTBEAT_nest0_loop_1_0_leftover(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem) {
  int64_t rc = 0;
  rc = HEARTBEAT_nest0_loop1_slice(cxts, constLiveIns, myIndex, tmem);
  if (rc > 0) {
    return;
  }

  cxts[LEVEL_ZERO * CACHELINE + START_ITER]++;
  rc = HEARTBEAT_nest0_loop0_slice(cxts, constLiveIns, myIndex, tmem);
  if (rc > 0) {
    return;
  }

  return;
}

} // namespace mandelbulb
