#include "floyd_warshall.hpp"
#if defined(HEARTBEAT_BRANCHES)
#include "heartbeat_branches.hpp"
#elif defined(HEARTBEAT_VERSIONING)
#include "heartbeat_versioning.hpp"
#else
#error "Need to specific the version of heartbeat, e.g., HEARTBEAT_BRANCHES, HEARTBEAT_VERSIONING
#endif

using namespace floyd_warshall;

int main() {
  setup();

#if defined(HEARTBEAT_BRANCHES)
  loop_dispatcher(&floyd_warshall_heartbeat_branches, dist, vertices);
#elif defined(HEARTBEAT_VERSIONING)
  loop_dispatcher(&floyd_warshall_heartbeat_versioning, dist, vertices);
#else
  #error "Need to specific the version of heartbeat, e.g., HEARTBEAT_BRANCHES, HEARTBEAT_VERSIONING
#endif

#if defined(TEST_CORRECTNESS)
  test_correctness(dist);
#endif

  finishup();

  return 0;

}
