#include <atomic>
#include <taskparts/benchmark.hpp>
#include "Heartbeats.hpp"
#include <pthread.h>

extern "C" {

  static pthread_spinlock_t lock;
  __attribute__((constructor))
  void lock_constructor () {
    if ( pthread_spin_init ( &lock, 0 ) != 0 ) {
        exit ( 1 );
    }
  }

  int loop_handler (
      long long int startIteration, 
      long long int maxIteration, 
      void *singleEnv, 
      void *reducibleEnv,
      long long int currentTaskID,
      void (*f)(int64_t, int64_t, void *, void *, int64_t)
      ) {
    static std::atomic_bool * me = taskparts::hardware_alarm_polling_interrupt::my_heartbeat_flag();

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
     * Step 3: split the remaining work
     */
    auto med = (maxIteration + startIteration)/2;

    printf("Loop_handler: Start\n");
    printf("Loop_handler:   Promotion\n");
    printf("Loop_handler:   startIteration = %lld\n", startIteration);
    printf("Loop_handler:   maxIteration = %lld\n", maxIteration);
    printf("Loop_handler:     Med = %lld\n", med);

    taskparts::tpalrts_promote_via_nativefj([&] {
      (*f)(startIteration, med, singleEnv, reducibleEnv, currentTaskID);
    }, [&] {
      (*f)(med, maxIteration, singleEnv, reducibleEnv, currentTaskID == 0 ? 1 : 0);
    }, [] { }, taskparts::bench_scheduler());

    return 1;
  }

  void loop_dispatcher (
    long long int startIteration, 
      long long int maxIteration, 
      void *singleEnv, 
      void *reducibleEnv,
      void (*f)(int64_t, int64_t, void *, void *, int64_t)

      ){
    taskparts::benchmark_nativeforkjoin([&] (auto sched) {
      (*f)(startIteration, maxIteration, singleEnv, reducibleEnv, 0);
    });
    return ;
  }

}
