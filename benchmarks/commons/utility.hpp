#pragma once

#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <taskparts/posix/steadyclock.hpp>

using namespace taskparts;

namespace utility {

void stats_begin();
void stats_end();

void run(std::function<void()> const &bench_body,
         std::function<void()> const &bench_start,
         std::function<void()> const &bench_end) {

#if defined(STATS)
  stats_begin();
#endif

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

  // UTILITY_BENCHMARK_VERBOSE
  bool verbose = false;
  if (const auto env_p = std::getenv("UTILITY_BENCHMARK_VERBOSE")) {
    verbose = std::stoi(env_p);
  }

  auto warmup = [&] {
    if (warmup_secs >= 0.0) {
      if (verbose) printf("======== WARMUP ========\n");
      auto warmup_start = steadyclock::now();
      while (steadyclock::since(warmup_start) < warmup_secs) {
        auto st = steadyclock::now();
        bench_body();
        auto el = steadyclock::since(st);
        if (verbose) printf("warmup_run %.3f\n", el);
      }
      if (verbose) printf ("======== END WARMUP ========\n");
    }
  };

  std::vector<double> exectimes;
  exectimes.reserve(repeat);

  bench_start();
  warmup();
  for (size_t i = 0; i < repeat; i++) {
    auto st = steadyclock::now();
    bench_body();
    auto el = steadyclock::since(st);
    exectimes.push_back(el);
  }
  bench_end();

  printf("[\n");
  for (size_t i = 0; i < exectimes.size(); i++) {
    printf("{\"exectime\": %.3f}", exectimes[i]);
    if (i + 1 != exectimes.size()) {
      printf(",\n");
    } else {
      printf("\n");
    }
  }
  printf("]\n");

#if defined(STATS)
  stats_end();
#endif

  return;
}

void stats_begin() {
  printf("{\n");

std::string benchmark_name = "";
  printf("\"benchmark\": \"%s\",\n", benchmark_name.c_str());

  printf("\"input_size\": ");
#if defined(INPUT_BENCHMARKING)
  printf("\"benchmarking\",\n");
#elif defined(INPUT_TPAL)
  printf("\"tpal\",\n");
#elif defined(INPUT_TESTING)
  printf("\"testing\",\n");
#endif

  printf("\"implementation\": ");
#if defined(USE_BASELINE)
  printf("\"baseline\",\n");
#elif defined(USE_OPENCILK)
  printf("\"opencilk\",\n");
#elif defined(USE_CILKPLUS)
  printf("\"cilkplus\",\n");
#elif defined(USE_OPENMP)
  printf("\"openmp\",\n");
  printf("\"schedule\": ");
#if defined(OMP_SCHEDULE_STATIC)
  printf("\"static\",\n");
#elif defined(OMP_SCHEDULE_DYNAMIC)
  printf("\"dynamic\",\n");
#elif defined(OMP_SCHEDULE_GUIDED)
  printf("\"guided\",\n");
#endif
  printf("\"nested_scheduling\": ");
#if defined(OMP_NESTED_SCHEDULING)
  printf("true,\n");
#else
  printf("false,\n");
#endif
#elif defined(USE_HB_MANUAL)
  printf("\"heartbeat_manual\",\n");
#elif defined(USE_HB_COMPILER)
  printf("\"heartbeat_compiler\",\n");
#endif

// following stats applies to heartbeat implementation only
#if defined(USE_HB_MANUAL) || defined(USE_HB_COMPILER)

  printf("\"enable_heartbeat_promotion\": ");
#if defined(ENABLE_HEARTBEAT_PROMOTION)
  printf("true,\n");
#else
  printf("false,\n");
#endif

// following stats applies only when heartbeat promotion is enabled
#if defined(ENABLE_HEARTBEAT_PROMOTION)

  printf("\"smallest_granularity\": %d,\n", SMALLEST_GRANULARITY);

  printf("\"signalism_mechanism\": ");
#if defined(ENABLE_ROLLFORWARD)
#if defined(USE_HB_KMOD)
  printf("\"kernel_module\",\n");
#else
  printf("\"interrupt_ping_thread\",\n");
#endif
#else
  printf("\"software_polling\",\n");
#endif

  printf("\"chunk_loop_iterations\": ");
#if defined(CHUNK_LOOP_ITERATIONS)
  printf("true,\n");
// TODO: find ways to handle multiple-level chunksize declarations
  printf("\"chunksizes\": [ ");
#if defined(CHUNKSIZE_0)
  printf("%d", CHUNKSIZE_0);
#endif
#if defined(CHUNKSIZE_1)
  printf(", %d", CHUNKSIZE_1);
#endif
#if defined(CHUNKSIZE_2)
  printf(", %d", CHUNKSIZE_2);
#endif
  printf(" ],\n");
#else
  printf("false\n");
#endif
#if !defined(ENABLE_ROLLFORWARD)
  printf("\"adaptive_chunksize_control\": ");
#if defined(CHUNK_LOOP_ITERATIONS) && defined(ADAPTIVE_CHUNKSIZE_CONTROL)
  printf("true,\n");
#else
  printf("false\n");
#endif
#endif

#endif // #if defined(ENABLE_HEARTBEAT_PROMOTION)

#endif  // #if defined(USE_HB_MANUAL) || defined(USE_HB_COMPILER)

  printf("\"test_correctness\": ");
#if defined(TEST_CORRECTNESS)
  printf("true,\n");
#else
  printf("false,\n");
#endif

  printf("\"runtime_reports\": \n");  // wrap the output from taskparts runtime
}

void stats_end() {
  printf("}\n");
}

} // namespace utility
