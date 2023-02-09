#pragma once

#include <cstdint>

extern "C" {

void loop_dispatcher(
  int64_t (*f)(void *, uint64_t, uint64_t, uint64_t),
  void *cxts,
  uint64_t myIndex,
  uint64_t startIter,
  uint64_t maxIter
);

int64_t loop_handler(
  void *cxts,
  uint64_t receivingLevel,
  #include "code_loop_handler_signature.hpp"
);

}
