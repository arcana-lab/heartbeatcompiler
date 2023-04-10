#pragma once

#include "loop_handler.hpp"
#include <cstdint>

namespace mandelbulb {

double x0, y0, z0, xstep, ystep, zstep;

void HEARTBEAT_nest0_loop2(int nx, int ny, int nz, int iterations) {
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

void HEARTBEAT_nest0_loop1(int nx, int ny, int nz, int iterations) {
  for(int j = 0; j < ny; ++j) {
    HEARTBEAT_nest0_loop2(nx, ny, nz, iterations);
  }
}

void HEARTBEAT_nest0_loop0(int nx, int ny, int nz, int iterations) {
  for (int i = 0; i < nx; ++i) {
    HEARTBEAT_nest0_loop1(nx, ny, nz, iterations);
  }
}

unsigned char* mandelbulb_compiler(int nx, int ny, int nz, int iterations,
				 double x0, double y0, double z0,
				 double x1, double y1, double z1) {
  unsigned char* output = (unsigned char*)malloc(nx * ny * nz * sizeof(unsigned char));
  xstep = (x1 - x0) / nx;
  ystep = (y1 - y0) / ny;
  zstep = (z1 - z0) / nz;
  HEARTBEAT_nest0_loop0(nx, ny, nz, iterations);
  return output;
}
  
} // namespace mandelbrot
