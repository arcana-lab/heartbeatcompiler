#include "heartbeat_manual.hpp"
#include "loop_handler.hpp"
#include <cstdint>

#define unlikely(expr) __builtin_expect(!!(expr), 0)
#define NUM_LEVELS_NEST0 2
#define LEVEL_ZERO 0
#define LEVEL_ONE 1
#define CACHELINE 8
#define START_ITER 0
#define MAX_ITER 1
#define LIVE_IN_ENV 2
#define CHUNKSIZE 4

namespace mandelbrot {

void HEARTBEAT_nest0_loop0(double x0, double y0, int width, int height, int max_depth, double xstep, double ystep, unsigned char *output);
void HEARTBEAT_nest0_loop1(double x0, double y0, int width, int max_depth, double xstep, double ystep, unsigned char *output, int j);

int64_t HEARTBEAT_nest0_loop0_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem);
int64_t HEARTBEAT_nest0_loop1_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem);
typedef int64_t (*sliceTasksPointer)(uint64_t *, uint64_t *, uint64_t, task_memory_t *);
sliceTasksPointer slice_tasks_nest0[2] = {
  &HEARTBEAT_nest0_loop0_slice,
  &HEARTBEAT_nest0_loop1_slice
};

void HEARTBEAT_nest0_loop_1_0_leftover(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem);
typedef void (*leftoverTasksPointer)(uint64_t *, uint64_t *, uint64_t, task_memory_t *);
leftoverTasksPointer leftover_tasks_nest0[1] = {
  &HEARTBEAT_nest0_loop_1_0_leftover
};

uint64_t leftover_selector_nest0(uint64_t receivingLevel, uint64_t splittingLevel) {
  return 0;
}

bool run_heartbeat = true;

// Outlined loops
void HEARTBEAT_nest0_loop0(double x0, double y0, int width, int height, int max_depth, double xstep, double ystep, unsigned char *output) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate const live-ins
    uint64_t constLiveIns[7];
    constLiveIns[0] = (uint64_t)&x0;
    constLiveIns[1] = (uint64_t)&y0;
    constLiveIns[2] = (uint64_t)width;
    constLiveIns[3] = (uint64_t)max_depth;
    constLiveIns[4] = (uint64_t)&xstep;
    constLiveIns[5] = (uint64_t)&ystep;
    constLiveIns[6] = (uint64_t)output;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS_NEST0 * CACHELINE];

    // set start/max iterations for loop0
    cxts[LEVEL_ZERO * CACHELINE + START_ITER] = (uint64_t)0;
    cxts[LEVEL_ZERO * CACHELINE + MAX_ITER] = (uint64_t)height;

#if defined(CHUNK_LOOP_ITERATIONS)
    // set the chunksize per loop level
    cxts[LEVEL_ZERO * CACHELINE + CHUNKSIZE] = CHUNKSIZE_0;
    cxts[LEVEL_ONE  * CACHELINE + CHUNKSIZE] = CHUNKSIZE_1;
#endif

    // allocate the task memory struct and initialize
    task_memory_t tmem;
    heartbeat_start(&tmem);

    // invoke nest0_loop0 in heartbeat form
    HEARTBEAT_nest0_loop0_slice(cxts, constLiveIns, 0, &tmem);

    run_heartbeat = true;
  } else {
    for(int j = 0; j < height; ++j) { // col loop
      HEARTBEAT_nest0_loop1(x0, y0, width, max_depth, xstep, ystep, output, j);
    }
  }
}

void HEARTBEAT_nest0_loop1(double x0, double y0, int width, int max_depth, double xstep, double ystep, unsigned char *output, int j) {
  for (int i = 0; i < width; ++i) { // row loop
    double z_real = x0 + i*xstep;
    double z_imaginary = y0 + j*ystep;
    double c_real = z_real;
    double c_imaginary = z_imaginary;
    double depth = 0;
    while(depth < max_depth) {
      if(z_real * z_real + z_imaginary * z_imaginary > 4.0) {
        break;
      }
      double temp_real = z_real*z_real - z_imaginary*z_imaginary;
      double temp_imaginary = 2.0*z_real*z_imaginary;
      z_real = c_real + temp_real;
      z_imaginary = c_imaginary + temp_imaginary;
      ++depth;
    }
    output[j*width + i] = static_cast<unsigned char>(static_cast<double>(depth) / max_depth * 255);
  }
}

// Transformed loops
int64_t HEARTBEAT_nest0_loop0_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem) {
  // load start/max iterations
  uint64_t startIter = cxts[LEVEL_ZERO * CACHELINE + START_ITER];
  uint64_t maxIter = cxts[LEVEL_ZERO * CACHELINE + MAX_ITER];

  // load const live-ins
  int width = (int)constLiveIns[2];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t low, high;
  // store &live-in as live-in environment for loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&low;

  uint64_t chunksize = cxts[LEVEL_ZERO * CACHELINE + CHUNKSIZE];
  for (; startIter < maxIter; startIter += chunksize) {
    low = startIter;
    high = maxIter < startIter + chunksize ? maxIter : startIter + chunksize;
    for (; low < high; low++) {
      // store current iteration for loop0
      cxts[LEVEL_ZERO * CACHELINE + START_ITER] = low;
      // set start/max iterations for loop1
      cxts[LEVEL_ONE * CACHELINE + START_ITER] = (uint64_t)0;
      cxts[LEVEL_ONE * CACHELINE + MAX_ITER] = (uint64_t)width;
      rc = HEARTBEAT_nest0_loop1_slice(cxts, constLiveIns, 0, tmem);
      if (rc > 0) {
        // update the exit condition here because there might
        // be tail work to finish
        high = low + 1;
      }
    }

    // exit the chunk execution when either
    // 1. heartbeat promotion happens at a higher nested level and in the process of returnning
    // 2. all iterations are finished
    if (rc > 0 || low == maxIter) {
      break;
    }

#if !defined(ENABLE_ROLLFORWARD)
    if (unlikely(heartbeat_polling(tmem))) {
      cxts[LEVEL_ZERO * CACHELINE + START_ITER] = low - 1;
      rc = loop_handler(
        cxts, constLiveIns, LEVEL_ZERO, NUM_LEVELS_NEST0, tmem,
        slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
      );
      if (rc > 0) {
        break;
      }
    }
#else
    cxts[LEVEL_ZERO * CACHELINE + START_ITER] = low - 1;
    __rf_handle_wrapper(
      rc, cxts, constLiveIns, LEVEL_ZERO, NUM_LEVELS_NEST0, tmem,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
    );
    if (rc > 0) {
      break;
    }
#endif
  }
#else
  // store &live-in as live-in environment for loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&startIter;

  for (; startIter < maxIter; startIter++) {
    // store current iteration for loop0
    cxts[LEVEL_ZERO * CACHELINE + START_ITER] = startIter;
    // set start/max iterations for loop1
    cxts[LEVEL_ONE * CACHELINE + START_ITER] = (uint64_t)0;
    cxts[LEVEL_ONE * CACHELINE + MAX_ITER] = (uint64_t)width;
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
#endif

  return rc - 1;
}

int64_t HEARTBEAT_nest0_loop1_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem) {
  // load start/max iterations
  uint64_t startIter = cxts[LEVEL_ONE * CACHELINE + START_ITER];
  uint64_t maxIter = cxts[LEVEL_ONE * CACHELINE + MAX_ITER];

  // load const live-ins
  double x0 = *(double *)constLiveIns[0];
  double y0 = *(double *)constLiveIns[1];
  int width = (int)constLiveIns[2];
  int max_depth = (int)constLiveIns[3];
  double xstep = *(double *)constLiveIns[4];
  double ystep = *(double *)constLiveIns[5];
  unsigned char *output = (unsigned char *)constLiveIns[6];

  // load live-ins
  int j = (int)*(uint64_t *)cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t chunksize = cxts[LEVEL_ONE * CACHELINE + CHUNKSIZE];
  for (; startIter < maxIter; startIter += chunksize) {
    uint64_t low = startIter;
    uint64_t high = maxIter < startIter + chunksize ? maxIter : startIter + chunksize;
    for (; low < high; low++) {
      double z_real = x0 + low*xstep;
      double z_imaginary = y0 + j*ystep;
      double c_real = z_real;
      double c_imaginary = z_imaginary;
      double depth = 0;
      while(depth < max_depth) {
        if(z_real * z_real + z_imaginary * z_imaginary > 4.0) {
          break;
        }
        double temp_real = z_real*z_real - z_imaginary*z_imaginary;
        double temp_imaginary = 2.0*z_real*z_imaginary;
        z_real = c_real + temp_real;
        z_imaginary = c_imaginary + temp_imaginary;
        ++depth;
      }
      output[j*width + low] = static_cast<unsigned char>(static_cast<double>(depth) / max_depth * 255);
    }

    if (low == maxIter) {
      break;
    }

#if !defined(ENABLE_ROLLFORWARD)
    if (unlikely(heartbeat_polling(tmem))) {
      cxts[LEVEL_ONE * CACHELINE + START_ITER] = low - 1;
      rc = loop_handler(
        cxts, constLiveIns, LEVEL_ONE, NUM_LEVELS_NEST0, tmem,
        slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
      );
      if (rc > 0) {
        break;
      }
    }
#else
    cxts[LEVEL_ONE * CACHELINE + START_ITER] = low - 1;
    __rf_handle_wrapper(
      rc, cxts, constLiveIns, LEVEL_ONE, NUM_LEVELS_NEST0, tmem,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
    );
    if (rc > 0) {
      break;
    }
#endif
  }
#else
  for(; startIter < maxIter; startIter++) {
    double z_real = x0 + startIter*xstep;
    double z_imaginary = y0 + j*ystep;
    double c_real = z_real;
    double c_imaginary = z_imaginary;
    double depth = 0;
    while(depth < max_depth) {
      if(z_real * z_real + z_imaginary * z_imaginary > 4.0) {
        break;
      }
      double temp_real = z_real*z_real - z_imaginary*z_imaginary;
      double temp_imaginary = 2.0*z_real*z_imaginary;
      z_real = c_real + temp_real;
      z_imaginary = c_imaginary + temp_imaginary;
      ++depth;
    }
    output[j*width + startIter] = static_cast<unsigned char>(static_cast<double>(depth) / max_depth * 255);

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
#endif

  return rc - 1;
}

// Leftover tasks
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

} // namespace mandelbrot
