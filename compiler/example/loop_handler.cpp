#include <atomic>
#include <taskparts/benchmark.hpp>
#include "Heartbeats.hpp"

extern "C" {

  int loop_handler (
      long long int startIteration, 
      long long int maxIteration, 
      void *env, 
      void (*f)(int64_t, int64_t, void *)
      ) {
    static std::atomic_bool * me = taskparts::hardware_alarm_polling_interrupt::my_heartbeat_flag();
    //hbm.addThread();
    static long long int currentIter = 0;

    /*
     * Check if an heartbeat happened.
     */
    if (!(*me)){
      return 0;
    }

    /*
     * A heartbeat happened.
     *
     * Step 1: reset the thread-specific heartbeat
     */
    (*me) = false;

    /*
     * Step 2: check if there is enough work
     */
    if ((maxIteration - startIteration) < 2){
      return 0;
    }

    /*
     * Step 3: avoid re-splitting the same work
     */
    if (startIteration == currentIter){
      return 0;
    }
    currentIter = startIteration;

    /*
     * Step 4: split the remaining work
     */
    auto med = (maxIteration + startIteration)/2;

    printf("Loop_handler: Start\n");
    printf("Loop_handler:   Promotion\n");
    printf("Loop_handler:   startIteration = %lld\n", startIteration);
    printf("Loop_handler:   maxIteration = %lld\n", maxIteration);
    printf("Loop_handler:     Med = %lld\n", med);

    taskparts::tpalrts_promote_via_nativefj([&] {
      (*f)(startIteration, med, env);
    }, [&] {
      (*f)(med, maxIteration, env);
    }, [] { }, taskparts::bench_scheduler());

    printf("Loop_handler: Exit\n");
    return 1;
  }

  void loop_dispatcher (
    long long int startIteration, 
      long long int maxIteration, 
      void *env, 
      void (*f)(int64_t, int64_t, void *)

      ){
    taskparts::benchmark_nativeforkjoin([&] (auto sched) {
      (*f)(startIteration, maxIteration, env);
    });
    return ;
  }

}
