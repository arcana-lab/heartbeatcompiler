MACROS_HEARTBEAT += -DUSE_HB_COMPILER $(MACROS)

# whether to enable heartbeat promotion
# options: true, false
ENABLE_HEARTBEAT_PROMOTION ?= true
ifeq ($(ENABLE_HEARTBEAT_PROMOTION), true)
	MACROS_HEARTBEAT += -DENABLE_HEARTBEAT_PROMOTION
else
	MACROS_HEARTBEAT += -DDISABLE_HEARTBEAT_PROMOTION
endif

# minimum remaining iteration size for heartbeat promotion
# options: unsigned integer >= 2
SMALLEST_GRANULARITY ?= 2
MACROS_HEARTBEAT += -DSMALLEST_GRANULARITY=$(SMALLEST_GRANULARITY)

# whether to chunk loop iterations before calling loop_handler
# options: true, false
CHUNK_LOOP_ITERATIONS ?= true
ifeq ($(CHUNK_LOOP_ITERATIONS), true)
	MACROS_HEARTBEAT += -DCHUNK_LOOP_ITERATIONS
endif

# include benchmark heartbeat specific settings
-include heartbeat.config

FRONTEND_FLAGS = -O1 -Xclang -disable-llvm-passes -emit-llvm

FLAGS_HEARTBEAT += $(FLAGS)
OPTS_HEARTBEAT += $(OPTS)
LIBS_HEARTBEAT += $(LIBS)

# ============================================
# Heartbeat
# ============================================
main.bc: main.cpp
	$(CXX) $(MACROS_HEARTBEAT) $(FRONTEND_FLAGS) $(FLAGS_HEARTBEAT) -c $^ -o $@
	llvm-dis $@

main_normalized.bc: main.bc
	noelle-norm $^ -o $@
	llvm-dis $@

heartbeat.bc: main_normalized.bc
	noelle-parallel-load -noelle-verbose=2 -load /home/yso0488/CAT/lib/HeartBeat.so -heartbeat $(PASS_ARGUMENTS_HEARTBEAT) $^ -o $@
	llvm-dis $@

heartbeat: heartbeat.bc bench.cpp loop_handler.cpp
	$(CXX) $(MACROS_HEARTBEAT) $(FLAGS_HEARTBEAT) $(OPTS_HEARTBEAT) $^ -o $@ $(LIBS_HEARTBEAT)

run_heartbeat: heartbeat
	TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_BASE_FREQUENCY_KHZ) ./$<
