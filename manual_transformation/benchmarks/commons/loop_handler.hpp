#pragma once

#include <cstdint>
#if defined(ENABLE_ROLLFORWARD)
#include <rollforward.h>
#endif

int64_t loop_handler_level1(void *cxt,
                            int64_t (*sliceTask)(void *, uint64_t, uint64_t, uint64_t),
                            uint64_t startIter, uint64_t maxIter);

#if defined(ENABLE_ROLLFORWARD)
void rollforward_handler_annotation __rf_handle_level1_wrapper(int64_t &rc,
                                                               void *cxt,
                                                               int64_t (*sliceTask)(void *, uint64_t, uint64_t, uint64_t),
                                                               uint64_t startIter, uint64_t maxIter);
#endif
