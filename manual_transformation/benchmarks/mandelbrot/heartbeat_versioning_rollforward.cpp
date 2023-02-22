#include "rollforward_handler.hpp"

#define CACHELINE     8
#define LIVE_IN_ENV   0

#define LEVEL_ZERO  0
#define LEVEL_ONE   1

#include "code_loop_slice_declaration.hpp"

extern int64_t HEARTBEAT_loop_1_0_leftover(uint64_t *, uint64_t *);
typedef int64_t(*leftoverTaskPointer)(uint64_t *, uint64_t *);
leftoverTaskPointer leftoverTasks[1] = {
  &HEARTBEAT_loop_1_0_leftover
};

int64_t HEARTBEAT_loop1_optimized(uint64_t *, uint64_t, uint64_t);
typedef int64_t(*leafTaskPointer)(uint64_t *, uint64_t, uint64_t);
leafTaskPointer leafTasks[1] = {
  &HEARTBEAT_loop1_optimized
};

extern uint64_t *constLiveIns;

// Transformed loop slice
int64_t HEARTBEAT_loop0_slice(uint64_t *cxts, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  int width = (int)constLiveIns[2];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_0 != 1
  int low, high;

  // store into live-in environment for loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&low;

  for (; startIter < maxIter; startIter += CHUNKSIZE_0) {
    low = (int)startIter;
    high = (int)std::min(maxIter, startIter + CHUNKSIZE_0);

    for (; low < high; low++) {
      rc = HEARTBEAT_loop1_slice(cxts, low, maxIter, 0, (uint64_t)width);
      if (rc > 0) {
        break;
      }
    }

    // exit the chunk execution when either
    // 1. heartbeat promotion happens at a higher nested level and in the process of returnning
    // 2. all iterations are finished
    if (rc > 0 || low == (int)maxIter) {
      break;
    }

#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_wrapper(rc, cxts, LEVEL_ZERO, leftoverTasks, nullptr, (uint64_t)low, maxIter, 0, 0);
#else
    rc = loop_handler(cxts, LEVEL_ZERO, leftoverTasks, nullptr, (uint64_t)low, maxIter, 0, 0);
#endif
    if (rc > 0) {
      break;
    }
  }
#else
  // store into live-in environment for loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&startIter;

  for(; (int)startIter < (int)maxIter; startIter++) {
    rc = HEARTBEAT_loop1_slice(cxts, startIter, maxIter, 0, (uint64_t)width);
    if (rc > 0) {
      break;
    }

#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_wrapper(rc, cxts, LEVEL_ZERO, leftoverTasks, nullptr, startIter, maxIter, 0, 0);
#else
    rc = loop_handler(cxts, LEVEL_ZERO, leftoverTasks, nullptr, startIter, maxIter, 0, 0);
#endif
    if (rc > 0) {
      break;
    }
  }
#endif

  return rc - 1;
}

int64_t HEARTBEAT_loop1_slice(uint64_t *cxts, uint64_t startIter0, uint64_t maxIter0, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  double x0 = *(double *)constLiveIns[0];
  double y0 = *(double *)constLiveIns[1];
  int width = (int)constLiveIns[2];
  int max_depth = (int)constLiveIns[3];
  double xstep = *(double *)constLiveIns[4];
  double ystep = *(double *)constLiveIns[5];
  unsigned char *output = (unsigned char *)constLiveIns[6];

  // load live-in environment
  int j = *(int *)cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_1 != 1
  for (; startIter < maxIter; startIter += CHUNKSIZE_1) {
    int low = (int)startIter;
    int high = (int)std::min(maxIter, startIter + CHUNKSIZE_1);

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
    if (low == (int)maxIter) {
      break;
    }

#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_wrapper(rc, cxts, LEVEL_ONE, leftoverTasks, leafTasks, startIter0, maxIter0, uint64_t(low - 1), maxIter);
#else
    rc = loop_handler(cxts, LEVEL_ONE, leftoverTasks, leafTasks, startIter0, maxIter0, uint64_t(low - 1), maxIter);
#endif
    if (rc > 0) {
      break;
    }
  }
#else
  for (; (int)startIter < (int)maxIter; startIter++) {
    double z_real = x0 + (int)startIter*xstep;
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
    output[j*width + (int)startIter] = static_cast<unsigned char>(static_cast<double>(depth) / max_depth * 255);

#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_wrapper(rc, cxts, LEVEL_ONE, leftoverTasks, leafTasks, startIter0, maxIter0, startIter, maxIter);
#else
    rc = loop_handler(cxts, LEVEL_ONE, leftoverTasks, leafTasks, startIter0, maxIter0, startIter, maxIter);
#endif
    if (rc > 0) {
      break;
    }
  }
#endif

  return rc - 1;
}

// Transformed optimized loop slice
int64_t HEARTBEAT_loop1_optimized(uint64_t *cxt, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  double x0 = *(double *)constLiveIns[0];
  double y0 = *(double *)constLiveIns[1];
  int width = (int)constLiveIns[2];
  int max_depth = (int)constLiveIns[3];
  double xstep = *(double *)constLiveIns[4];
  double ystep = *(double *)constLiveIns[5];
  unsigned char *output = (unsigned char *)constLiveIns[6];

  // load live-in environment
  int j = *(int *)cxt[LIVE_IN_ENV];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_1 != 1
  for (; startIter < maxIter; startIter += CHUNKSIZE_1) {
    int low = (int)startIter;
    int high = (int)std::min(maxIter, startIter + CHUNKSIZE_1);

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
    if (low == (int)maxIter) {
      break;
    }

#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_optimized_wrapper(rc, cxt, (uint64_t)(low - 1), maxIter, &HEARTBEAT_loop1_optimized);
#else
    rc = loop_handler_optimized(cxt, (uint64_t)(low - 1), maxIter, &HEARTBEAT_loop1_optimized);
#endif
    if (rc > 0) {
      break;
    }
  }
#else
  for (; (int)startIter < (int)maxIter; startIter++) {
    double z_real = x0 + (int)startIter*xstep;
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
    output[j*width + (int)startIter] = static_cast<unsigned char>(static_cast<double>(depth) / max_depth * 255);

#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_optimized_wrapper(rc, cxt, startIter, maxIter, &HEARTBEAT_loop1_optimized);
#else
    rc = loop_handler_optimized(cxt, startIter, maxIter, &HEARTBEAT_loop1_optimized);
#endif
    if (rc > 0) {
      break;
    }
  }
#endif

  return rc - 1;
}
