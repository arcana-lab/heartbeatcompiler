#pragma once

#include "loop_handler.hpp"

namespace floyd_warshall {

void HEARTBEAT_nest0_loop0(int *dist, int vertices, int via);

inline
void floyd_warshall_hb_manual(int *dist, int vertices) {
  for(int via = 0; via < vertices; via++) {
    HEARTBEAT_nest0_loop0(dist, vertices, via);
  }
}

} // namespace floyd_warshall
