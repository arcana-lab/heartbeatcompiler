#pragma once

#include <rollforward.h>
#include <cstdint>
#include <algorithm>

extern
int64_t loop_handler(
  uint64_t *cxts,
  uint64_t receivingLevel,
  #include "code_loop_handler_signature.hpp"
);

extern
int64_t loop_handler_optimized(
  uint64_t *cxt,
  uint64_t startIter,
  uint64_t maxIter,
  #include "code_optimized_loop_handler_signature.hpp"
);

#if defined(ENABLE_ROLLFORWARD)
void rollforward_handler_annotation __rf_handle_wrapper(
  int64_t &rc,
  uint64_t *cxts,
  uint64_t receivingLevel,
  #include "code_loop_handler_signature.hpp"
) {
  #include "code_loop_handler_invocation.hpp"
  rollbackward
}

void rollforward_handler_annotation __rf_handle_optimized_wrapper(
  int64_t &rc,
  uint64_t *cxt,
  uint64_t startIter,
  uint64_t maxIter,
  #include "code_optimized_loop_handler_signature.hpp"
) {
  rc = loop_handler_optimized(cxt, startIter, maxIter, leafTask);
  rollbackward
}
#endif
