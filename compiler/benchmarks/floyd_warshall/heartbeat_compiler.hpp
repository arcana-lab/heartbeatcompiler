#pragma once

#include "loop_handler.hpp"
#include <cstdint>

#define SUB(array, row_sz, i, j) (array[i * row_sz + j])

namespace floyd_warshall {

void HEARTBEAT_nest0_loop0(int *dist, uint64_t vertices, uint64_t via);
void HEARTBEAT_nest0_loop1(int *dist, uint64_t vertices, uint64_t via, uint64_t from);

void floyd_warshall_hb_compiler(int* dist, int vertices) {
  for(int via = 0; via < vertices; via++) {
    HEARTBEAT_nest0_loop0(dist, vertices, via);
  }
}

// Outlined loops
// void HEARTBEAT_nest0_loop0(int *dist, int vertices, int via) {
void HEARTBEAT_nest0_loop0(int *dist, uint64_t vertices, uint64_t via) {
  // for(int from = 0; from < vertices; from++) {
  for(uint64_t from = 0; from < vertices; from++) {
    HEARTBEAT_nest0_loop1(dist, vertices, via, from);
  }
}

// void HEARTBEAT_nest0_loop1(int *dist, int vertices, int via, int from) {
void HEARTBEAT_nest0_loop1(int *dist, uint64_t vertices, uint64_t via, uint64_t from) {
  // for(int to = 0; to < vertices; to++) {
  for(uint64_t to = 0; to < vertices; to++) {
    if ((from != to) && (from != via) && (to != via)) {
      SUB(dist, vertices, from, to) = 
        std::min(SUB(dist, vertices, from, to), 
                  SUB(dist, vertices, from, via) + SUB(dist, vertices, via, to));
    }
  }
}

} // namespace floyd_warshall
