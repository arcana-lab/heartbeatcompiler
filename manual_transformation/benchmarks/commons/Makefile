CC = clang
CXX = clang++

# whether to collect and print kernel_exectime when program finishes
# options: true, false
COLLECT_KERNEL_TIME ?= true
ifeq ($(COLLECT_KERNEL_TIME), true)
	MACROS += -DCOLLECT_KERNEL_TIME
endif

# size of input, either benchmarking or testing
# options: benchmarking, testing
INPUT_SIZE ?= benchmarking
ifeq ($(INPUT_SIZE), benchmarking)
	MACROS += -DINPUT_BENCHMARKING
else
	MACROS += -DINPUT_TESTING
endif

# whether to test the correctness of any implementation
# options: true, false
TEST_CORRECTNESS ?= false
ifeq ($(TEST_CORRECTNESS), true)
	MACROS += -DTEST_CORRECTNESS
endif

# include benchmark specific settings
benchmark = $(notdir $(shell pwd))
-include $(benchmark).config

FLAGS = --std=c++17
OPTS = -O3 -march=native -fno-vectorize -fno-slp-vectorize
LIBS = -lm -lstdc++

# ======================
# Baseline
# ======================
MACROS_BASELINE = $(MACROS)
FLAGS_BASELINE = $(FLAGS)
OPTS_BASLINE = $(OPTS)
LIBS_BASELINE = $(LIBS)

baseline: baseline.cpp
	$(CXX) $(MACROS_BASELINE) $(FLAGS_BASELINE) $(OPTS_BASLINE) $^ -o $@ $(LIBS_BASELINE)

run_baseline: baseline
	./$<

# ======================
# OpenCilk
# ======================
OPENCILK_COMPILER ?= /nfs-scratch/yso0488/opencilk/build/bin/bin/opencilk++

MACROS_OPENCILK += $(MACROS) -DUSE_OPENCILK
FLAGS_OPENCILK = $(FLAGS) -fopencilk
OPTS_OPENCILK = $(OPTS)
LIBS_OPENCILK = $(LIBS)

opencilk: opencilk.cpp
	$(OPENCILK_COMPILER) $(MACROS_OPENCILK) $(FLAGS_OPENCILK) $(OPTS_OPENCILK) $^ -o $@ $(LIBS_OPENCILK)

run_opencilk: opencilk
	./$<

# ======================
# OpenMP
# ======================
MACROS_OPENMP = $(MACROS) -DUSE_OPENMP
FLAGS_OPENMP = $(FLAGS) -fopenmp
OPTS_OPENMP = $(OPTS)
LIBS_OPENMP = $(LIBS) -fopenmp

openmp: openmp.cpp
	$(CXX) $(MACROS_OPENMP) $(FLAGS_OPENMP) $(OPTS_OPENMP) $^ -o $@ $(LIBS_OPENMP)

run_openmp: openmp
	./$<

openmp_dynamic: openmp.cpp
	$(CXX) $(MACROS_OPENMP) -DOMP_DYNAMIC $(FLAGS_OPENMP) $(OPTS_OPENMP) $^ -o $@ $(LIBS_OPENMP)

run_openmp_dynamic: openmp_dynamic
	./$<

openmp_guided: openmp.cpp
	$(CXX) $(MACROS_OPENMP) -DOMP_GUIDED $(FLAGS_OPENMP) $(OPTS_OPENMP) $^ -o $@ $(LIBS_OPENMP)

run_openmp_guided: openmp_guided
	./$<

# ======================
# Heartbeat
# ======================
CPU_BASE_FREQUENCY_KHZ ?= 2800000
TASKPARTS_PATH ?= ../../runtime
TASKPARTS_INCLUDE_PATH = $(TASKPARTS_PATH)/include
TASKPARTS_INCLUDES = $(TASKPARTS_INCLUDE_PATH) $(HWLOC_INCLUDE_PATH)

MACROS_HEARTBEAT += $(MACROS)

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

# which loop_handler version to use, with/without live-out environment
# options: true, false
HANDLE_LIVE_OUT ?= false
ifeq ($(HANDLE_LIVE_OUT), false)
	MACROS_HEARTBEAT += -DLOOP_HANDLER_WITHOUT_LIVE_OUT
else
	MACROS_HEARTBEAT += -DLOOP_HANDLER_WITH_LIVE_OUT
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
LEFTOVER_SPLITTABLE ?= false
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
	-DTASKPARTS_TPALRTS_HARDWARE_ALARM_POLLING \
	-DTASKPARTS_STATS \
	# -DTASKPARTS_LOG
MACROS_HEARTBEAT += $(TASKPARTS_FLAGS)

FLAGS_HEARTBEAT = \
	$(FLAGS) \
	-I $(TASKPARTS_INCLUDES)

OPTS_HEARTBEAT = \
	$(OPTS) \
	-fno-stack-protector \
	-fno-asynchronous-unwind-tables \
	-fomit-frame-pointer

ifeq (, $(shell which jemalloc-config))
	CUSTOM_MALLOC =
else
	JEMALLOCLD = $(shell jemalloc-config --libdir)
	JEMALLOCLS = $(shell jemalloc-config --libs)
	CUSTOM_MALLOC = -L$(JEMALLOCLD) -Wl,-rpath,$(JEMALLOCLD) $(JEMALLOCLS) -ljemalloc
endif

LIBS_HEARTBEAT = \
	-pthread \
	$(HWLOC_LIBRARY_PATH) \
	$(CUSTOM_MALLOC) # needs to go last, or else it won't be used

# ======================
# Heartbeat::Branches
# ======================
heartbeat_branches: heartbeat.cpp
	$(CXX) -DHEARTBEAT_BRANCHES $(MACROS_HEARTBEAT) $(FLAGS_HEARTBEAT) $(OPTS_HEARTBEAT) $^ -o $@ $(LIBS_HEARTBEAT)

run_heartbeat_branches: heartbeat_branches
	export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_BASE_FREQUENCY_KHZ); ./$<

# ======================
# Heartbeat::Versioning
# ======================
heartbeat_versioning: heartbeat.cpp
	$(CXX) -DHEARTBEAT_VERSIONING $(MACROS_HEARTBEAT) $(FLAGS_HEARTBEAT) $(OPTS_HEARTBEAT) $^ -o $@ $(LIBS_HEARTBEAT)

run_heartbeat_versioning: heartbeat_versioning
	export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_BASE_FREQUENCY_KHZ); ./$<

# ======================
# Heartbeat:Evaluation
# ======================
condor:
	cd eval ; condor_submit job.con ;

# ======================
# Other
# ======================
clean:
	rm -f baseline opencilk openmp openmp_dynamic openmp_guided ;
	rm -f heartbeat_branches heartbeat_versioning ;
	rm -f correctness *.json ;

.PHONY: clean
