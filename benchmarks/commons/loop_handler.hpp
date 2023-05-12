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

// Memorization information passed by the hb task and
// accessed and analyzed by the runtime functions
struct heartbeat_memory_t {
  uint64_t startingLevel;
#if !defined(ENABLE_ROLLFORWARD) && defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
  uint64_t polling_count;
#endif
};

void heartbeat_reset(heartbeat_memory_t *hbmem, uint64_t startingLevel);

#if !defined(ENABLE_ROLLFORWARD)
bool heartbeat_polling(heartbeat_memory_t *hbmem);
#endif

int64_t loop_handler(
  uint64_t *cxts,
  uint64_t *constLiveIns,
  uint64_t receivingLevel,
  uint64_t numLevels,
  heartbeat_memory_t *hbmem,
  int64_t (*slice_tasks[])(uint64_t *, uint64_t *, uint64_t, heartbeat_memory_t *),
  void (*leftover_tasks[])(uint64_t *, uint64_t *, uint64_t, heartbeat_memory_t *),
  uint64_t (*leftover_selector)(uint64_t, uint64_t)
);

#if defined(ENABLE_ROLLFORWARD)
void rollforward_handler_annotation __rf_handle_wrapper(
  int64_t &rc,
  uint64_t *cxts,
  uint64_t *constLiveIns,
  uint64_t receivingLevel,
  uint64_t numLevels,
  heartbeat_memory_t *hbmem,
  int64_t (*slice_tasks[])(uint64_t *, uint64_t *, uint64_t, heartbeat_memory_t *),
  void (*leftover_tasks[])(uint64_t *, uint64_t *, uint64_t, heartbeat_memory_t *),
  uint64_t (*leftover_selector)(uint64_t, uint64_t)
);
#endif

}
