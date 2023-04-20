#pragma once

#include <cstdint>
#include <functional>
#if defined(ENABLE_ROLLFORWARD)
#include <rollforward.h>
#endif

extern "C" {

void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end);

bool heartbeat_polling();

int64_t loop_handler(
  uint64_t *cxts,
  uint64_t receivingLevel,
  int64_t (*slice_tasks[])(uint64_t *, uint64_t),
  void (*leftover_tasks[])(uint64_t *, uint64_t),
  uint64_t (*leftover_selector)(uint64_t, uint64_t)
);

#if defined(ENABLE_ROLLFORWARD)
void rollforward_handler_annotation __rf_handle_wrapper(
  int64_t &rc,
  uint64_t *cxts,
  uint64_t receivingLevel,
  int64_t (*slice_tasks[])(uint64_t *, uint64_t),
  void (*leftover_tasks[])(uint64_t *, uint64_t),
  uint64_t (*leftover_selector)(uint64_t, uint64_t)
);
#endif

// int64_t loop_handler_level1(
//   void *cxt,
//   int64_t (*sliceTask)(void *, uint64_t, uint64_t, uint64_t),
//   uint64_t startIter, uint64_t maxIter
// );

// #if defined(ENABLE_ROLLFORWARD)
// void rollforward_handler_annotation __rf_handle_level1_wrapper(
//   int64_t &rc,
//   void *cxt,
//   int64_t (*sliceTask)(void *, uint64_t, uint64_t, uint64_t),
//   uint64_t startIter, uint64_t maxIter
// );
// #endif

// int64_t loop_handler_level2(
//   void *cxts,
//   uint64_t receivingLevel,
//   void (*slice_tasks[])(void *, uint64_t, uint64_t, uint64_t),
//   void (*leftover_tasks[])(void *, uint64_t, void *),
//   uint64_t (*leftover_selector)(uint64_t, uint64_t),
//   uint64_t s0, uint64_t m0,
//   uint64_t s1, uint64_t m1
// );

// #if defined(ENABLE_ROLLFORWARD)
// void rollforward_handler_annotation __rf_handle_level2_wrapper(
//   int64_t &rc,
//   void *cxts,
//   uint64_t receivingLevel,
//   void (*slice_tasks[])(void *, uint64_t, uint64_t, uint64_t),
//   void (*leftover_tasks[])(void *, uint64_t, void *),
//   uint64_t (*leftover_selector)(uint64_t, uint64_t),
//   uint64_t s0, uint64_t m0,
//   uint64_t s1, uint64_t m1
// );
// #endif

}
