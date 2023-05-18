#pragma once

#include "loop_handler.hpp"

namespace mandelbulb {

void HEARTBEAT_nest0_loop0(double x0, double y0, double z0, int nx, int ny, int nz, int iterations, double power, double xstep, double ystep, double zstep, unsigned char *output);

inline
unsigned char * mandelbulb_hb_manual(double x0, double y0, double z0,
                                     double x1, double y1, double z1,
                                     int nx, int ny, int nz, int iterations,
                                     double power) {
  unsigned char* output = (unsigned char*)malloc(nx * ny * nz * sizeof(unsigned char));
  double xstep = (x1 - x0) / nx;
  double ystep = (y1 - y0) / ny;
  double zstep = (z1 - z0) / nz;
  HEARTBEAT_nest0_loop0(x0, y0, z0, nx, ny, nz, iterations, power, xstep, ystep, zstep, output);
  return output;
}

} // namespace mandelbulb
