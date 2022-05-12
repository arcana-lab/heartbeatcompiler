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
      void *env, 
      long long int currentTaskID,
      void (*f)(int64_t, int64_t, void *, int64_t)
      ) {
    static std::atomic_bool * me = taskparts::hardware_alarm_polling_interrupt::my_heartbeat_flag();
    //hbm.addThread();
    static long long int currentIter = 0;
    static int64_t taskID = 0;

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

    /*
    printf("Loop_handler: Start\n");
    printf("Loop_handler:   Promotion\n");
    printf("Loop_handler:   startIteration = %lld\n", startIteration);
    printf("Loop_handler:   maxIteration = %lld\n", maxIteration);
    printf("Loop_handler:     Med = %lld\n", med);
    */

    /*
     * Spawn a new task.
     */
    int64_t newTaskID;
    pthread_spin_lock(&lock);
    taskID++;
    newTaskID = taskID;
    pthread_spin_unlock(&lock);

    taskparts::tpalrts_promote_via_nativefj([&] {
      (*f)(startIteration, med, env, currentTaskID);
    }, [&] {
      (*f)(med, maxIteration, env, newTaskID);
    }, [] { }, taskparts::bench_scheduler());

    return 1;
  }

  void loop_dispatcher (
    long long int startIteration, 
      long long int maxIteration, 
      void *env, 
      void (*f)(int64_t, int64_t, void *, int64_t)

      ){
    taskparts::benchmark_nativeforkjoin([&] (auto sched) {
      (*f)(startIteration, maxIteration, env, 0);
    });
    return ;
  }

}
