#pragma once

#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <taskparts/posix/steadyclock.hpp>

using namespace taskparts::steadyclock;

namespace utility {

void run(std::function<void()> const &bench_body,
         std::function<void()> const &bench_start,
         std::function<void()> const &bench_end) {

  // UTILITY_BENCHMARK_NUM_REPEAT
  size_t repeat = 1;
  if (const auto env_p = std::getenv("UTILITY_BENCHMARK_NUM_REPEAT")) {
    repeat = std::stoi(env_p);
  }

  // UTILITY_BENCHMARK_WARMUP_SECS
  double warmup_secs = 0.0;
  if (const auto env_p = std::getenv("UTILITY_BENCHMARK_WARMUP_SECS")) {
    warmup_secs = (double)std::stoi(env_p);
  }

  bool verbose = false;
  if (const auto env_p = std::getenv("UTILITY_BENCHMARK_VERBOSE")) {
    verbose = std::stoi(env_p);
  }

  auto warmup = [&] {
    if (warmup_secs >= 0.0) {
      if (verbose) printf("======== WARMUP ========\n");
      auto warmup_start = taskparts::steadyclock::now();
      while (taskparts::steadyclock::since(warmup_start) < warmup_secs) {
        auto st = taskparts::steadyclock::now();
        bench_body();
        auto el = taskparts::steadyclock::since(st);
        if (verbose) printf("warmup_run %.3f\n", el);
      }
      if (verbose) printf ("======== END WARMUP ========\n");
    }
  };

  bench_start();
  warmup();
  for (size_t i = 0; i < repeat; i++) {
    auto st = taskparts::steadyclock::now();
    bench_body();
    auto el = taskparts::steadyclock::since(st);
    fprintf(stdout, "\"exectime\": %.3f\n", el);
  }
  bench_end();

  return;
}

} // namespace utility
