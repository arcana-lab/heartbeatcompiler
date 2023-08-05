#pragma once

#include "loop_handler.hpp"

namespace mandelbrot {

unsigned char * mandelbrot_hb_manual(double x0, double y0, double x1, double y1,
                                     int height, int width, int max_depth);

} // namespace mandelbrot
