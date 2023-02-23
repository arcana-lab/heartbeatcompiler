#pragma once

#include <cstdint>
#include <functional>

extern "C" {

void loop_dispatcher(
  std::function<void()> const &lambda
);

int64_t loop_handler(
  void *cxts,
  uint64_t receivingLevel,
  #include "code_loop_handler_signature.hpp"
);

}
