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

}
