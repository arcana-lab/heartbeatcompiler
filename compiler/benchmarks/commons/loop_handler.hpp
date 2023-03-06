#pragma once

#include <cstdint>
#include <functional>

extern "C" {

void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end);

int64_t loop_handler_level1(void *cxt,
                            int64_t (*sliceTask)(void *, uint64_t, uint64_t, uint64_t),
                            uint64_t startIter, uint64_t maxIter);

}
