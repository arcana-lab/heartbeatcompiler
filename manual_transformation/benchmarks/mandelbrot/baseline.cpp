#include "mandelbrot.hpp"

using namespace mandelbrot;

int main() {
  setup();

  output = mandelbrot_serial(_mb_x0, _mb_y0, _mb_x1, _mb_y1, _mb_width, _mb_height, _mb_max_depth);

  finishup(output);
  
  return 0;
}
