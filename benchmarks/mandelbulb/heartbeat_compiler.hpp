#pragma once

#include "loop_handler.hpp"
#include <cstdint>
#include <cmath>

namespace mandelbulb {

void HEARTBEAT_nest0_loop0(double x0, double y0, double z0,
                           uint64_t nx, uint64_t ny, uint64_t nz, uint64_t iterations, double power,
                           double xstep, double ystep, double zstep, unsigned char *output);
void HEARTBEAT_nest0_loop1(double x0, double y0, double z0,
                           uint64_t ny, uint64_t nz, uint64_t iterations, double power,
                           double xstep, double ystep, double zstep, unsigned char *output,
                           uint64_t i);
void HEARTBEAT_nest0_loop2(double x0, double y0, double z0,
                           uint64_t ny, uint64_t nz, uint64_t iterations, double power,
                           double xstep, double ystep, double zstep, unsigned char *output,
                           uint64_t i, uint64_t j);

unsigned char * mandelbulb_hb_compiler(double x0, double y0, double z0,
                                       double x1, double y1, double z1,
                                       int nx, int ny, int nz, int iterations, double power) {
  unsigned char* output = (unsigned char*)malloc(nx * ny * nz * sizeof(unsigned char));
  double xstep = (x1 - x0) / nx;
  double ystep = (y1 - y0) / ny;
  double zstep = (z1 - z0) / nz;
  HEARTBEAT_nest0_loop0(x0, y0, z0, nx, ny, nz, iterations, power, xstep, ystep, zstep, output);
  return output;
}

// void HEARTBEAT_nest0_loop0(double x0, double y0, double z0,
//                            int nx, int ny, int nz, int iterations, double power,
//                            double xstep, double ystep, double zstep, unsigned char *output) {
void HEARTBEAT_nest0_loop0(double x0, double y0, double z0,
                           uint64_t nx, uint64_t ny, uint64_t nz, uint64_t iterations, double power,
                           double xstep, double ystep, double zstep, unsigned char *output) {
  // for (int i = 0; i < nx; ++i) {
  for (uint64_t i = 0; i < nx; ++i) {
    HEARTBEAT_nest0_loop1(x0, y0, z0, ny, nz, iterations, power, xstep, ystep, zstep, output, i);
  }
}

// void HEARTBEAT_nest0_loop1(double x0, double y0, double z0,
//                            int ny, int nz, int iterations, double power,
//                            double xstep, double ystep, double zstep, unsigned char *output,
//                            int i) {
void HEARTBEAT_nest0_loop1(double x0, double y0, double z0,
                           uint64_t ny, uint64_t nz, uint64_t iterations, double power,
                           double xstep, double ystep, double zstep, unsigned char *output,
                           uint64_t i) {
  // for(int j = 0; j < ny; ++j) {
  for(uint64_t j = 0; j < ny; ++j) {
    HEARTBEAT_nest0_loop2(x0, y0, z0, ny, nz, iterations, power, xstep, ystep, zstep, output, i, j);
  }
}

// void HEARTBEAT_nest0_loop2(double x0, double y0, double z0,
//                            int ny, int nz, int iterations, double power,
//                            double xstep, double ystep, double zstep, unsigned char *output,
//                            int i, int j) {
void HEARTBEAT_nest0_loop2(double x0, double y0, double z0,
                           uint64_t ny, uint64_t nz, uint64_t iterations, double power,
                           double xstep, double ystep, double zstep, unsigned char *output,
                           uint64_t i, uint64_t j) {
  // for (int k = 0; k < nz; ++k) {
  for (uint64_t k = 0; k < nz; ++k) {
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

} // namespace mandelbulb
