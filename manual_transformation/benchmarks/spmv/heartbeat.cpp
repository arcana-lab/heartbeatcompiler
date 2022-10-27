#include "spmv.hpp"
#if defined(HEARTBEAT_BRANCHES)
#include "heartbeat_branches.hpp"
#elif defined(HEARTBEAT_VERSIONING)
#include "heartbeat_versioning.hpp"
#else
#error "Need to specific the version of heartbeat, e.g., HEARTBEAT_BRANCHES, HEARTBEAT_VERSIONING
#endif

using namespace spmv;

int main() {
  setup();

#if defined(HEARTBEAT_BRANCHES)
  loop_dispatcher(&spmv_heartbeat_branches, val, row_ptr, col_ind, x, y, nb_rows);
#elif defined(HEARTBEAT_VERSIONING)
  loop_dispatcher(&spmv_heartbeat_versioning, val, row_ptr, col_ind, x, y, nb_rows);
#else
  #error "Need to specific the version of heartbeat, e.g., HEARTBEAT_BRANCHES, HEARTBEAT_VERSIONING
#endif

#if defined(TEST_CORRECTNESS)
  test_correctness(y);
#endif

  finishup();

  return 0;
}