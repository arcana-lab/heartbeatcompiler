#pragma once

#include "loop_handler.hpp"

#define CACHELINE     8
#define LIVE_IN_ENV   0
#define START_ITER    0
#define MAX_ITER      1

#define NUM_LEVELS  2
#define LEVEL_ZERO  0
#define LEVEL_ONE   1

/*
 * Benchmark-specific variable indicating the level of nested loop
 * needs to be defined outside the namespace
 */
uint64_t numLevels;

/*
 * User defined function to determine the index of the leftover task
 * needs to be defined outside the namespace
 */
uint64_t getLeftoverTaskIndex(uint64_t splittingLevel, uint64_t myLevel) {
  return 0;
}

/*
 * User defined function to determine the index of the leaf task
 * needs to be defined outside the namespace
 */
uint64_t getLeafTaskIndex(uint64_t myLevel) {
  return 0;
}

void mandelbrot_heartbeat_versioning(double, double, double, double, int, int, int, unsigned char *&);
void HEARTBEAT_loop0(double, double, int, int, int, double, double, unsigned char *);
void HEARTBEAT_loop1(double, double, int, int, double, double, unsigned char *, int);
int64_t HEARTBEAT_loop_1_0_leftover(uint64_t *, uint64_t *);

static bool run_heartbeat = true;
uint64_t *constLiveIns;

// Entry function for the benmark
void mandelbrot_heartbeat_versioning(double x0, double y0, double x1, double y1,
                                     int width, int height, int max_depth, unsigned char *&_output) {
  double xstep = (x1 - x0) / width;
  double ystep = (y1 - y0) / height;
  unsigned char* output = (unsigned char*)malloc(width * height * sizeof(unsigned char));
  if (run_heartbeat) {
    run_heartbeat = false;

    // initialize number of levels for this nested loop
    numLevels = NUM_LEVELS;

    // allocate const live-ins
    constLiveIns = (uint64_t *)alloca(sizeof(uint64_t) * 7);
    constLiveIns[0] = (uint64_t)&x0;
    constLiveIns[1] = (uint64_t)&y0;
    constLiveIns[2] = (uint64_t)width;
    constLiveIns[3] = (uint64_t)max_depth;
    constLiveIns[4] = (uint64_t)&xstep;
    constLiveIns[5] = (uint64_t)&ystep;
    constLiveIns[6] = (uint64_t)output;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS * CACHELINE];

    // invoke loop0 in heartbeat form
    HEARTBEAT_loop0_slice(cxts, 0, (uint64_t)height);

    run_heartbeat = true;
  } else {
    HEARTBEAT_loop0(x0, y0, width, height, max_depth, xstep, ystep, output);
  }

  _output = output;
  return;
}

// Outlined loops
void HEARTBEAT_loop0(double x0, double y0, int width, int height, int max_depth, double xstep, double ystep, unsigned char* output) {
  for(int j = 0; j < height; ++j) { // col loop
    HEARTBEAT_loop1(x0, y0, width, max_depth, xstep, ystep, output, j);
  }

  return;
}

void HEARTBEAT_loop1(double x0, double y0, int width, int max_depth, double xstep, double ystep, unsigned char *output, int j) {
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

  return;
}

// Leftover loops
int64_t HEARTBEAT_loop_1_0_leftover(uint64_t *cxts, uint64_t *itersArr) {
  // load startIter and maxIter
  uint64_t startIter = itersArr[LEVEL_ONE * 2 + START_ITER];
  uint64_t maxIter   = itersArr[LEVEL_ONE * 2 + MAX_ITER];

#ifndef LEFTOVER_SPLITTABLE

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

  return 0;

#else

  return HEARTBEAT_loop1_slice(cxts, 0, 0, startIter, maxIter);

#endif
}
