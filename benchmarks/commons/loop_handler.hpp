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

/*
 * Memorization information passed by the hb task and
 * accessed and analyzed by the runtime functions
 */
struct task_memory_t {
  uint64_t startingLevel;
#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t chunksize;
  uint64_t remaining_chunksize;
#if defined(ENABLE_SOFTWARE_POLLING) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
  uint64_t polling_count;
#endif
#endif
};

#if defined(CHUNK_LOOP_ITERATIONS)
uint64_t get_chunksize(task_memory_t *tmem);

bool update_and_has_remaining_chunksize(task_memory_t *tmem, uint64_t iterations, uint64_t chunksize);
#endif

void heartbeat_start(task_memory_t *tmem, uint64_t num_iterations);

#if defined(ENABLE_SOFTWARE_POLLING)
bool heartbeat_polling(task_memory_t *tmem);
#endif

int64_t loop_handler(
  uint64_t *cxts,
  uint64_t *constLiveIns,
  uint64_t receivingLevel,
  uint64_t numLevels,
  task_memory_t *tmem,
  int64_t (*slice_tasks[])(uint64_t *, uint64_t *, uint64_t, task_memory_t *),
  void (*leftover_tasks[])(uint64_t *, uint64_t *, uint64_t, task_memory_t *),
  uint64_t (*leftover_selector)(uint64_t, uint64_t)
);

#if defined(ENABLE_ROLLFORWARD)
void rollforward_handler_annotation __rf_handle_wrapper(
  int64_t &rc,
  uint64_t *cxts,
  uint64_t *constLiveIns,
  uint64_t receivingLevel,
  uint64_t numLevels,
  task_memory_t *tmem,
  int64_t (*slice_tasks[])(uint64_t *, uint64_t *, uint64_t, task_memory_t *),
  void (*leftover_tasks[])(uint64_t *, uint64_t *, uint64_t, task_memory_t *),
  uint64_t (*leftover_selector)(uint64_t, uint64_t)
);
#endif

}
