#pragma once

#include "loop_handler.hpp"

namespace mandelbrot {

void HEARTBEAT_nest0_loop0(double x0, double y0, int width, int height, int max_depth, double xstep, double ystep, unsigned char *output);

inline
unsigned char * mandelbrot_hb_manual(double x0, double y0, double x1, double y1,
                                     int width, int height, int max_depth) {
  double xstep = (x1 - x0) / width;
  double ystep = (y1 - y0) / height;
  unsigned char* output = (unsigned char*)malloc(width * height * sizeof(unsigned char));
  HEARTBEAT_nest0_loop0(x0, y0, width, height, max_depth, xstep, ystep, output);
  return output;
}

} // namespace mandelbrot
