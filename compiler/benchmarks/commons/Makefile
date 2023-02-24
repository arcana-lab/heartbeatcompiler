CPU_BASE_FREQUENCY_KHZ ?= 2800000
TASKPARTS_PATH ?= ../../../runtime
TASKPARTS_INCLUDE = $(TASKPARTS_PATH)/include
TASKPARTS_INCLUDES = $(TASKPARTS_INCLUDE) $(HWLOC_INCLUDE_PATH)
ROLLFORWARDXX_PATH = $(TASKPARTS_PATH)/benchmark/tpal
ROLLFORWARDXX = $(ROLLFORWARDXX_PATH)/rollforward++
ROLLFORWARD_PATH ?= ../../../rollforward/src
ROLLFORWARD_INCLUDE = $(ROLLFORWARD_PATH)/x64

MACROS_HEARTBEAT += $(MACROS)

-include heartbeat.config

# whether to chunk loop iterations before calling loop_handler
# options: true, false
CHUNK_LOOP_ITERATIONS ?= true
ifeq ($(CHUNK_LOOP_ITERATIONS), true)
	MACROS_HEARTBEAT += -DCHUNK_LOOP_ITERATIONS
endif

# minimum remaining iteration size for heartbeat promotion
# options: unsigned integer >= 2
SMALLEST_GRANULARITY ?= 2
MACROS_HEARTBEAT += -DSMALLEST_GRANULARITY=$(SMALLEST_GRANULARITY)

# whether to enable heartbeat promotion
# options: true, false
ENABLE_HEARTBEAT_PROMOTION ?= true
ifeq ($(ENABLE_HEARTBEAT_PROMOTION), true)
	MACROS_HEARTBEAT += -DENABLE_HEARTBEAT_PROMOTION
else
	MACROS_HEARTBEAT += -DDISABLE_HEARTBEAT_PROMOTION
endif

# report time spent in software heartbeat polling
# options: true, false
COLLECT_HEARTBEAT_POLLING_TIME ?= false
ifeq ($(ENABLE_HEARTBEAT_PROMOTION), true)
	ifeq ($(COLLECT_HEARTBEAT_POLLING_TIME), true)
		MACROS_HEARTBEAT += -DCOLLECT_HEARTBEAT_POLLING_TIME
	endif
endif

# report time overhead spent in heartbeat promotion
# options: true, false
COLLECT_HEARTBEAT_PROMOTION_TIME ?= false
ifeq ($(ENABLE_HEARTBEAT_PROMOTION), true)
	ifeq ($(COLLECT_HEARTBEAT_PROMOTION_TIME), true)
		MACROS_HEARTBEAT += -DCOLLECT_HEARTBEAT_PROMOTION_TIME
	endif
endif

# whether the leftover task is further splittable through heartbeat promotion
# options: true, false
LEFTOVER_SPLITTABLE ?= true
ifeq ($(LEFTOVER_SPLITTABLE), true)
	MACROS_HEARTBEAT += -DLEFTOVER_SPLITTABLE
else
	MACROS_HEARTBEAT += -DLEFTOVER_UNSPLITTABLE
endif

# macros used for taskparts runtime
TASKPARTS_PLATFORM ?= -DTASKPARTS_POSIX -DTASKPARTS_X64
TASKPARTS_FLAGS = \
	$(TASKPARTS_PLATFORM) \
	-DTASKPARTS_TPALRTS \
	-DTASKPARTS_STATS \
	# -DTASKPARTS_LOG
ifeq ($(ENABLE_ROLLFORWARD), false)
	TASKPARTS_FLAGS += -DTASKPARTS_TPALRTS_HARDWARE_ALARM_POLLING
endif
MACROS_HEARTBEAT += $(TASKPARTS_FLAGS)

FRONTEND_FLAGS = -O1 -Xclang -disable-llvm-passes -emit-llvm

FLAGS_HEARTBEAT = \
	$(FLAGS) \
	-I $(TASKPARTS_INCLUDES) \
	-I $(ROLLFORWARD_INCLUDE)

OPTS_HEARTBEAT = \
	$(OPTS) \
	-fno-stack-protector \
	-fno-asynchronous-unwind-tables \
	-fomit-frame-pointer

LIBS_HEARTBEAT = \
	-pthread \
	$(HWLOC_LIBRARY_PATH) \
	$(CUSTOM_MALLOC) # needs to go last, or else it won't be used

# ============================================
# Code Generation
# ============================================
code_generation: generate_code.sh level.config live_out.config
	./$< `cat level.config` `cat live_out.config`

# ============================================
# Heartbeat::Versioning
# ============================================
heartbeat.bc: heartbeat.cpp
	make code_generation ;
	$(CXX) -DHEARTBEAT_VERSIONING $(MACROS_HEARTBEAT) $(FRONTEND_FLAGS) $(FLAGS_HEARTBEAT) -c $^ -o $@
	llvm-dis $@

heartbeat_normalized.bc: heartbeat.bc
	noelle-norm $^ -o $@
	llvm-dis $@

heartbeat_versioning.bc: heartbeat_normalized.bc
	noelle-parallel-load -noelle-verbose=2 -load /home/yso0488/CAT/lib/HeartBeat.so -heartbeat $(PASS_ARGUMENTS_HEARTBEAT) $^ -o $@
	llvm-dis $@

heartbeat_versioning: heartbeat_versioning.bc bench.cpp loop_handler.cpp
ifeq ($(ENABLE_ROLLFORWARD), true)
	make heartbeat_versioning_rf
else
	$(CXX) -DHEARTBEAT_VERSIONING $(MACROS_HEARTBEAT) $(FLAGS_HEARTBEAT) $(OPTS_HEARTBEAT) $^ -o $@ $(LIBS_HEARTBEAT)
endif

run_heartbeat_versioning: heartbeat_versioning
	TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_BASE_FREQUENCY_KHZ) ./$<

# ============================================
# Heartbeat::Versioning (Rollforward)
# ============================================
heartbeat_versioning_rollforward.s: heartbeat_versioning.bc
	$(CXX) $(MACROS_HEARTBEAT) $(FLAGS_HEARTBEAT) $(OPTS_HEARTBEAT) $< -S -o $@

heartbeat_versioning.rf.s: heartbeat_versioning_rollforward.s
	ROLLFORWARD_PATH=$(ROLLFORWARD_PATH) ./$(ROLLFORWARDXX) $< $@

heartbeat_versioning.rf.o: heartbeat_versioning.rf.s
	$(CXX) -I $(ROLLFORWARD_INCLUDE) $< -c -o $@

rollforward.o: $(ROLLFORWARD_INCLUDE)/rollforward.c
	$(CXX) $< -c -o $@

heartbeat_versioning_rf: heartbeat_versioning.rf.o rollforward.o spmv.cpp loop_handler.cpp
	$(CXX) $(MACROS_HEARTBEAT) $(FLAGS_HEARTBEAT) $(OPTS_HEARTBEAT) $^ -o $@ $(LIBS_HEARTBEAT)
	cp $@ heartbeat_versioning
