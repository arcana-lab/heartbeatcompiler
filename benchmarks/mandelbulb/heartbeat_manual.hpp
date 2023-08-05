#pragma once

#include "loop_handler.hpp"

namespace mandelbulb {

unsigned char * mandelbulb_hb_manual(double x0, double y0, double z0,
                                     double x1, double y1, double z1,
                                     int nx, int ny, int nz, int iterations,
                                     double power);

} // namespace mandelbulb
