#!/bin/bash

level=$1
handle_live_out=$2
path=./

./$path/generate_loop_slice_declaration.sh                $level $handle_live_out > code_loop_slice_declaration.hpp
./$path/generate_loop_handler_signature.sh                $level $handle_live_out > code_loop_handler_signature.hpp
./$path/generate_splitting_level_determination.sh         $level                  > code_splitting_level_determination.hpp
./$path/generate_iteration_state_snapshot.sh              $level                  > code_iteration_state_snapshot.hpp
./$path/generate_slice_context_construction.sh                   $handle_live_out > code_slice_context_construction.hpp
./$path/generate_leftover_context_construction.sh                $handle_live_out > code_leftover_context_construction.hpp
./$path/generate_slice_task_invocation.sh                 $level $handle_live_out > code_slice_task_invocation.hpp
./$path/generate_leftover_task_invocation.sh              $level $handle_live_out > code_leftover_task_invocation.hpp
