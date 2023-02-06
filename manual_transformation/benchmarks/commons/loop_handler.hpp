#pragma once

#include <taskparts/benchmark.hpp>
#include <alloca.h>

#ifndef SMALLEST_GRANULARITY
  #error "Macro SMALLEST_GRANULARITY undefined"
#endif

#if defined(COLLECT_HEARTBEAT_POLLING_TIME) || defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
#include <pthread.h>
#include <stdio.h>

#define RDTSC(rdtsc_val_) do { \
  uint32_t rdtsc_hi_, rdtsc_lo_; \
  __asm__ volatile ("rdtsc" : "=a" (rdtsc_lo_), "=d" (rdtsc_hi_)); \
  rdtsc_val_ = (uint64_t)rdtsc_hi_ << 32 | rdtsc_lo_; \
} while (0)

auto cpu_frequency_khz = taskparts::get_cpu_frequency_khz();
auto num_workers = taskparts::perworker::id::get_nb_workers();

#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
volatile pthread_spinlock_t lock;
static volatile uint64_t useful_cycles = 0;
static volatile uint64_t useful_invocations = 0;
static volatile uint64_t wasted_cycles = 0;
static volatile uint64_t wasted_invocations = 0;

void collect_heartbeat_polling_time_init() {
  pthread_spin_init(&lock, 0);
}

void collect_heartbeat_polling_time_print() {
  printf("========= Heartbeat Polling Time Summary =========\n");
  printf("cpu_frequency_khz: %lu\n", cpu_frequency_khz);
  printf("num_workers: %zu\n", num_workers);

  printf("\n");

  printf("useful_cycles: %lu\n", useful_cycles);
  printf("useful_invocations: %lu\n", useful_invocations);
  printf("useful_cycles_per_invocation: %.2f\n", useful_invocations == 0 ? 0.00 : (double)useful_cycles/(double)useful_invocations);
  printf("useful_seconds: %.2f\n", useful_invocations == 0 ? 0.00 : (double)useful_cycles/((double)cpu_frequency_khz * 1000)/(double)num_workers);

  printf("\n");

  printf("wasted_cycles: %lu\n", wasted_cycles);
  printf("wasted_invocations: %lu\n", wasted_invocations);
  printf("wasted_cycles_per_invocation: %.2f\n", wasted_invocations == 0 ? 0.00 : (double)wasted_cycles/(double)wasted_invocations);
  printf("wasted_seconds: %.2f\n", wasted_invocations == 0 ? 0.00 : (double)wasted_cycles/((double)cpu_frequency_khz * 1000)/(double)num_workers);
  printf("==================================================\n");
}
#endif

#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
volatile pthread_spinlock_t promotion_lock;
static volatile uint64_t promotion_generic_cycles = 0;
static volatile uint64_t promotion_generic_counts = 0;
static volatile uint64_t promotion_optimized_cycles = 0;
static volatile uint64_t promotion_optimized_counts = 0;

void collect_heartbeat_promotion_time_init() {
  pthread_spin_init(&promotion_lock, 0);
}

void collect_heartbeat_promotion_time_print() {
  printf("======== Heartbeat Promotion Time Summary ========\n");
  printf("cpu_frequency_khz: %lu\n", cpu_frequency_khz);
  printf("num_workers: %zu\n", num_workers);

  printf("\n");

  printf("promotion_generic_cycles: %lu\n", promotion_generic_cycles);
  printf("promotion_generic_counts: %lu\n", promotion_generic_counts);
  printf("promotion_generic_cycles_per_count: %.2f\n", promotion_generic_counts == 0 ? 0.00 : (double)promotion_generic_cycles/(double)promotion_generic_counts);
  printf("promotion_generic_seconds: %.2f\n", promotion_generic_counts == 0 ? 0.00 : (double)promotion_generic_cycles/((double)cpu_frequency_khz * 1000)/(double)num_workers);

  printf("\n");

  printf("promotion_optimized_cycles: %lu\n", promotion_optimized_cycles);
  printf("promotion_optimized_counts: %lu\n", promotion_optimized_counts);
  printf("promotion_optimized_cycles_per_count: %.2f\n", promotion_optimized_counts == 0 ? 0.00 : (double)promotion_optimized_cycles/(double)promotion_optimized_counts);
  printf("promotion_optimized_seconds: %.2f\n", promotion_optimized_counts == 0 ? 0.00 : (double)promotion_optimized_cycles/((double)cpu_frequency_khz * 1000)/(double)num_workers);
  printf("==================================================\n");
}
#endif
#endif

#define CACHELINE     8
#define LIVE_IN_ENV   0
#define LIVE_OUT_ENV  1
#define START_ITER    0
#define MAX_ITER      1

/*
 * Benchmark-specific variable indicating the level of nested loop
 * This variable should be defined inside heartbeat_*.hpp
 */
extern uint64_t numLevels;

#if defined(HEARTBEAT_BRANCHES)

#elif defined(HEARTBEAT_VERSIONING)

#include "code_loop_slice_declaration.hpp"

#if !defined(LOOP_HANDLER_OPTIMIZED)

/*
 * The benchmark-specific function to determine the leftover task to use
 * giving the splittingLevel and receivingLevel
 * This function should be defined inside heartbeat_versioning.hpp
 */
uint64_t getLeftoverTaskIndex(uint64_t splittingLevel, uint64_t myLevel);

/*
 * The benchmark-specific function to determine the leaf task to use
 * giving the receivingLevel
 * This function should be defined inside heartbeat_versioning.hpp
 */
uint64_t getLeafTaskIndex(uint64_t myLevel);

/*
 * Generic loop_handler function for the versioning version
 * 1. if no heartbeat promotion happens, return 0
 *    if heartbeat promotion happens, 
 *      return the number of levels that maxIter needs to be reset
 * 2. caller side should prepare the live-out environment (if any) for kids,
 *    loop_handler does simply copy the existing live-out environments
 */
int64_t loop_handler(
  uint64_t *cxts,
  uint64_t receivingLevel,
  #include "code_loop_handler_signature.hpp"
) {
#if defined(DISABLE_HEARTBEAT_PROMOTION)
  return 0;
#endif

#if !defined(ENABLE_ROLLFORWARD)
  /*
   * Determine whether to promote since last promotion
   */
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  uint64_t start, end;
  RDTSC(start);
#endif
  auto &p = taskparts::prev.mine();
  auto n = taskparts::cycles::now();
  if ((p + taskparts::kappa_cycles) > n) {
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  RDTSC(end);
  pthread_spin_lock(&lock);
  wasted_cycles += end - start;
  wasted_invocations += 1;
  pthread_spin_unlock(&lock);
#endif
    return 0;
  }
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  RDTSC(end);
  pthread_spin_lock(&lock);
  useful_cycles += end - start;
  useful_invocations += 1;
  pthread_spin_unlock(&lock);
#endif
  p = n;
#endif

  /*
   * Decide the splitting level
   */
  uint64_t splittingLevel = numLevels;
  #include "code_splitting_level_determination.hpp"

  /*
   * No more work to split at any level
   */
  if (splittingLevel == numLevels) {
    return 0;
  }

#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  uint64_t promotion_start, promotion_end;
  RDTSC(promotion_start);
#endif
  /*
   * Snapshot global iteration state
   */
  uint64_t *itersArr = (uint64_t *)alloca(sizeof(uint64_t) * numLevels * 2);
  uint64_t index = 0;
  #include "code_iteration_state_snapshot.hpp"

  /*
   * Allocate cxts for both tasks
   */
  uint64_t *cxtsFirst   = (uint64_t *)alloca(sizeof(uint64_t) * numLevels * CACHELINE);
  uint64_t *cxtsSecond  = (uint64_t *)alloca(sizeof(uint64_t) * numLevels * CACHELINE);

  /*
   * Determine the splitting point of the remaining iterations
   */
  uint64_t med = (itersArr[splittingLevel * 2 + START_ITER] + 1 + itersArr[splittingLevel * 2 + MAX_ITER]) / 2;

  /*
   * Reconstruct the context at the splittingLevel for both tasks
   */
  #include "code_slice_context_construction.hpp"

  if (splittingLevel == receivingLevel) { // no leftover task needed
    /*
     * Split the rest of work depending on whether splitting the leaf loop
     */
    if (receivingLevel + 1 != numLevels) {
#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  RDTSC(promotion_end);
  pthread_spin_lock(&promotion_lock);
  promotion_generic_cycles += promotion_end - promotion_start;
  promotion_generic_counts += 1;
  pthread_spin_unlock(&promotion_lock);
#endif
      #include "code_slice_task_invocation.hpp"

    } else {
      /*
       * Determine which optimized leaf task to run
       */
      uint64_t leafTaskIndex = getLeafTaskIndex(receivingLevel);

#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  RDTSC(promotion_end);
  pthread_spin_lock(&promotion_lock);
  promotion_generic_cycles += promotion_end - promotion_start;
  promotion_generic_counts += 1;
  pthread_spin_unlock(&promotion_lock);
#endif
      #include "code_leaf_task_invocation.hpp"
    }

  } else { // build up the leftover task
    /*
     * Allocate context for leftover task
     */
    uint64_t *cxtsLeftover = (uint64_t *)alloca(sizeof(uint64_t) * numLevels * CACHELINE);

    /*
     * Reconstruct the context starting from splittingLevel + 1 up to receivingLevel for leftover task
     */
    for (uint64_t level = splittingLevel + 1; level <= receivingLevel; level++) {
      uint64_t index = level * CACHELINE;
      #include "code_leftover_context_construction.hpp"

      /*
       * Rreset global iters for leftover task
       */
      itersArr[level * 2 + START_ITER] += 1;
    }

    /*
     * Determine which leftover task to run
     */
    uint64_t leftoverTaskIndex = getLeftoverTaskIndex(splittingLevel, receivingLevel);

#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  RDTSC(promotion_end);
  pthread_spin_lock(&promotion_lock);
  promotion_generic_cycles += promotion_end - promotion_start;
  promotion_generic_counts += 1;
  pthread_spin_unlock(&promotion_lock);
#endif
    #include "code_leftover_task_invocation.hpp"
  }

  /*
   * Return the number of levels that maxIter needs to be reset
   */
  return receivingLevel - splittingLevel + 1;
}

#endif

int64_t loop_handler_optimized(
  uint64_t *cxt,
  uint64_t startIter,
  uint64_t maxIter,
  #include "code_optimized_loop_handler_signature.hpp"
) {
#if defined(DISABLE_HEARTBEAT_PROMOTION)
  return 0;
#endif

#if !defined(ENABLE_ROLLFORWARD)
  /*
   * Determine whether to promote since last promotion
   */
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  uint64_t start, end;
  RDTSC(start);
#endif
  auto &p = taskparts::prev.mine();
  auto n = taskparts::cycles::now();
  if ((p + taskparts::kappa_cycles) > n) {
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  RDTSC(end);
  pthread_spin_lock(&lock);
  wasted_cycles += end - start;
  wasted_invocations += 1;
  pthread_spin_unlock(&lock);
#endif
    return 0;
  }
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  RDTSC(end);
  pthread_spin_lock(&lock);
  useful_cycles += end - start;
  useful_invocations += 1;
  pthread_spin_unlock(&lock);
#endif
  p = n;
#endif

  /*
   * Determine if there's more work for splitting
   */
  if ((maxIter - startIter) <= SMALLEST_GRANULARITY) {
    return 0;
  }

#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  uint64_t promotion_start, promotion_end;
  RDTSC(promotion_start);
#endif
  /*
   * Allocate and construct cxts for both tasks
   */
  uint64_t cxtFirst[1 * CACHELINE];
  uint64_t cxtSecond[1 * CACHELINE];

  uint64_t med  = (startIter + 1 + maxIter) / 2;

  #include "code_optimized_slice_context_construction.hpp"

#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  RDTSC(promotion_end);
  pthread_spin_lock(&promotion_lock);
  promotion_optimized_cycles += promotion_end - promotion_start;
  promotion_optimized_counts += 1;
  pthread_spin_unlock(&promotion_lock);
#endif
  #include "code_optimized_leaf_task_invocation.hpp"

  return 1;
}

#endif
