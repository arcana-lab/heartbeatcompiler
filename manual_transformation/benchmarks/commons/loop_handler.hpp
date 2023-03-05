#pragma once

#include <cstdint>
#if defined(ENABLE_ROLLFORWARD)
#include <rollforward.h>
#endif

int loop_handler_level1(uint64_t *cxt,
                        int (*sliceTask)(uint64_t *, uint64_t, uint64_t, uint64_t),
                        uint64_t startIter, uint64_t maxIter);

#if defined(ENABLE_ROLLFORWARD)
void rollforward_handler_annotation __rf_handle_level1_wrapper(int &rc,
                                                               uint64_t *cxt,
                                                               int (*sliceTask)(uint64_t *, uint64_t, uint64_t, uint64_t),
                                                               uint64_t startIter, uint64_t maxIter);
#endif
