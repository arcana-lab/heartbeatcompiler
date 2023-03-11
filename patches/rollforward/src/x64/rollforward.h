#pragma once

#include <stdint.h>
#include <stdio.h>

struct rollforward_struct {
  void* from;
  void* to;
};

extern
uint64_t rollforward_table_size;
extern
uint64_t rollback_table_size;
extern
struct rollforward_struct rollforward_table[];
extern
struct rollforward_struct rollback_table[];

extern
void try_to_initiate_rollforward(void **rip);
extern
void* try_to_initiate_rollbackward(void* ra_dst);

#define rollforward_handler_annotation \
   __attribute__((noinline))

//#define rollforward_handler_annotation	\
//   __attribute__((preserve_all, noinline))

#define rollbackward \
  { \
  void* ra_dst = __builtin_return_address(0); \
  void* ra_src = try_to_initiate_rollbackward(ra_dst); \
  if (ra_src != nullptr) { \
    void* fa = __builtin_frame_address(0); \
    void** rap = (void**)((char*)fa + 8); \
    *rap = ra_src; \
  } else { \
    for (uint64_t i = 0; i < rollback_table_size; i++) { \
      if (rollforward_table[i].from == ra_dst) { \
	ra_src = rollforward_table[i].to; \
	break; \
      } \
    } \
    if (ra_src == nullptr) { \
      printf("found no entry in rollforward table!\n"); \
    } \
    exit(1); \
  } }

#define rollforward_branch_unlikely(x)    __builtin_expect(!!(x), 0)
