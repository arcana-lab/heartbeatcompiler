#pragma once

#include "loop_handler.hpp"
#include <cstdint>

namespace mandelbrot {

void HEARTBEAT_nest0_loop0(double x0, double y0, uint64_t height, uint64_t width, uint64_t max_depth, double xstep, double ystep, unsigned char *output);
void HEARTBEAT_nest0_loop1(double x0, double y0, uint64_t width, uint64_t max_depth, double xstep, double ystep, unsigned char *output, uint64_t j);

unsigned char * mandelbrot_hb_compiler(double x0, double y0, double x1, double y1,
                                       int height, int width, int max_depth) {
  double xstep = (x1 - x0) / width;
  double ystep = (y1 - y0) / height;
  unsigned char* output = (unsigned char*)malloc(width * height * sizeof(unsigned char));
  HEARTBEAT_nest0_loop0(x0, y0, height, width, max_depth, xstep, ystep, output);
  return output;
}

// Outlined loops
// void HEARTBEAT_nest0_loop0(double x0, double y0, int height, int width, int max_depth, double xstep, double ystep, unsigned char *output) {
void HEARTBEAT_nest0_loop0(double x0, double y0, uint64_t height, uint64_t width, uint64_t max_depth, double xstep, double ystep, unsigned char *output) {
  // for(int j = 0; j < height; ++j) { // col loop
  for(uint64_t j = 0; j < height; ++j) { // col loop
    HEARTBEAT_nest0_loop1(x0, y0, width, max_depth, xstep, ystep, output, j);
  }
}

// void HEARTBEAT_nest0_loop1(double x0, double y0, int width, int max_depth, double xstep, double ystep, unsigned char *output, int j) {
void HEARTBEAT_nest0_loop1(double x0, double y0, uint64_t width, uint64_t max_depth, double xstep, double ystep, unsigned char *output, uint64_t j) {
  // for (int i = 0; i < width; ++i) { // row loop
  for (uint64_t i = 0; i < width; ++i) { // row loop
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

} // namespace mandelbrot
