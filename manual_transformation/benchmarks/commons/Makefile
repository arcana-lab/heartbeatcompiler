CC = clang
CXX = clang++

# whether to collect and print kernel_exectime when program finishes
# options: true, false
COLLECT_KERNEL_TIME ?= true
ifeq ($(COLLECT_KERNEL_TIME), true)
	MACROS += -DCOLLECT_KERNEL_TIME
endif

# size of input, either benchmarking, testing or tpal
# options: benchmarking, testing, tpal
INPUT_SIZE ?= testing
ifeq ($(INPUT_SIZE), benchmarking)
	MACROS += -DINPUT_BENCHMARKING
else ifeq ($(INPUT_SIZE), testing)
	MACROS += -DINPUT_TESTING
else ifeq ($(INPUT_SIZE), tpal)
	MACROS += -DINPUT_TPAL
endif

# whether to test the correctness of any implementation
# options: true, false
TEST_CORRECTNESS ?= false
ifeq ($(TEST_CORRECTNESS), true)
	MACROS += -DTEST_CORRECTNESS
endif

# whether to enable the rollforward compilation
# options: true, false
ENABLE_ROLLFORWARD ?= false
ifeq ($(ENABLE_ROLLFORWARD), true)
	CXX = g++
	MACROS_HEARTBEAT += -DENABLE_ROLLFORWARD
else
	MACROS_HEARTBEAT += -DDISABLE_ROLLFORWARD
endif

# include benchmark specific settings
benchmark = $(notdir $(shell pwd))
-include $(benchmark).config

FLAGS = --std=c++17
OPTS = -O3 -march=native
ifeq ($(CXX), clang++)
	OPTS += -fno-vectorize -fno-slp-vectorize
else ifeq ($(CXX), g++)
	OPTS += -fno-tree-vectorize
endif
LIBS = -lm -lstdc++

ifeq (, $(shell which jemalloc-config))
	CUSTOM_MALLOC =
else
	JEMALLOCLD = $(shell jemalloc-config --libdir)
	JEMALLOCLS = $(shell jemalloc-config --libs)
	CUSTOM_MALLOC = -L$(JEMALLOCLD) -Wl,-rpath,$(JEMALLOCLD) $(JEMALLOCLS) -ljemalloc
endif

# ======================
# Baseline
# ======================
MACROS_BASELINE += $(MACROS)
FLAGS_BASELINE += $(FLAGS)
OPTS_BASLINE += $(OPTS)
LIBS_BASELINE += $(LIBS)

baseline: baseline.cpp
	$(CXX) $(MACROS_BASELINE) $(FLAGS_BASELINE) $(OPTS_BASLINE) $^ -o $@ $(LIBS_BASELINE)

run_baseline: baseline
	./$<

# ======================
# OpenCilk
# ======================
OPENCILK_COMPILER ?= /nfs-scratch/yso0488/opencilk/build/bin/bin/opencilk++

MACROS_OPENCILK += $(MACROS) -DUSE_OPENCILK
FLAGS_OPENCILK += $(FLAGS) -fopencilk
OPTS_OPENCILK += $(OPTS)
ifneq ($(CXX), clang++)
	OPTS_OPENCILK += -fno-vectorize -fno-slp-vectorize
endif
LIBS_OPENCILK += \
	$(LIBS) \
	$(CUSTOM_MALLOC) # needs to go last, or else it won't be used

opencilk: opencilk.cpp
	$(OPENCILK_COMPILER) $(MACROS_OPENCILK) $(FLAGS_OPENCILK) $(OPTS_OPENCILK) $^ -o $@ $(LIBS_OPENCILK)

run_opencilk: opencilk
	./$<

# ======================
# OpenMP
# ======================
MACROS_OPENMP += $(MACROS) -DUSE_OPENMP
FLAGS_OPENMP += $(FLAGS) -fopenmp
OPTS_OPENMP += $(OPTS)
LIBS_OPENMP += $(LIBS) -fopenmp

OMP_SCHEDULE ?= STATIC
ifeq ($(OMP_SCHEDULE), STATIC)
	MACROS_OPENMP += -DOMP_STATIC
else ifeq ($(OMP_SCHEDULE), DYNAMIC)
	MACROS_OPENMP += -DOMP_DYNAMIC
else ifeq ($(OMP_SCHEDULE), GUIDED)
	MACROS_OPENMP += -DOMP_GUIDED
endif

openmp: openmp.cpp
	$(CXX) $(MACROS_OPENMP) $(FLAGS_OPENMP) $(OPTS_OPENMP) $^ -o $@ $(LIBS_OPENMP)

run_openmp: openmp
	./$<

# ======================
# Heartbeat
# ======================
CPU_BASE_FREQUENCY_KHZ ?= 2800000
TASKPARTS_PATH ?= ../../runtime
TASKPARTS_INCLUDE = $(TASKPARTS_PATH)/include
TASKPARTS_INCLUDES = $(TASKPARTS_INCLUDE) $(HWLOC_INCLUDE_PATH)
ROLLFORWARDXX_PATH = $(TASKPARTS_PATH)/benchmark/tpal
ROLLFORWARDXX = $(ROLLFORWARDXX_PATH)/rollforward++
ROLLFORWARD_PATH ?= ../../rollforward/src
ROLLFORWARD_INCLUDE = $(ROLLFORWARD_PATH)/x64

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

# ======================
# Code Generation
# ======================
code_generation: generate_code.sh level.config live_out.config
	./$< `cat level.config` `cat live_out.config`

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
heartbeat_versioning: heartbeat.cpp heartbeat_versioning_rollforward.cpp
	make code_generation
ifeq ($(ENABLE_ROLLFORWARD), true)
	make heartbeat_versioning_rf
else
	$(CXX) -DHEARTBEAT_VERSIONING $(MACROS_HEARTBEAT) $(FLAGS_HEARTBEAT) $(OPTS_HEARTBEAT) $^ $(ROLLFORWARD_SEPARATOR) -o $@ $(LIBS_HEARTBEAT)
endif

run_heartbeat_versioning: heartbeat_versioning
	export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_BASE_FREQUENCY_KHZ); ./$<

# ============================================
# Heartbeat::Versioning (Rollforward)
# ============================================
heartbeat_versioning_rollforward.s: heartbeat_versioning_rollforward.cpp
	$(CXX) -DHEARTBEAT_VERSIONING $(MACROS_HEARTBEAT) $(FLAGS_HEARTBEAT) $(OPTS_HEARTBEAT) $< -S -o $@

heartbeat_versioning.rf.s: heartbeat_versioning_rollforward.s
	ROLLFORWARD_PATH=$(ROLLFORWARD_PATH) ./$(ROLLFORWARDXX) $< $@

heartbeat_versioning.rf.o: heartbeat_versioning.rf.s
	$(CXX) -I $(ROLLFORWARD_INCLUDE) $< -c -o $@

rollforward.o: $(ROLLFORWARD_INCLUDE)/rollforward.c
	$(CXX) $< -c -o $@

heartbeat_versioning_rf: heartbeat.cpp heartbeat_versioning.rf.o rollforward.o
	$(CXX) -DHEARTBEAT_VERSIONING $(MACROS_HEARTBEAT) $(FLAGS_HEARTBEAT) $(OPTS_HEARTBEAT) $^ -o $@ $(LIBS_HEARTBEAT)
	cp $@ heartbeat_versioning

# ======================
# Evaluation
# ======================
condor:
	cd eval ; condor_submit job.con ;

condor_clean:
	cd eval ; rm -rf condor.* ;

# ======================
# Other
# ======================
clean:
	rm -rf baseline opencilk openmp ;
	rm -rf heartbeat_branches heartbeat_versioning ;
	rm -rf heartbeat_versioning_rollforward.s heartbeat_versioning.rf.s heartbeat_versioning.rf.o rollforward.o heartbeat_versioning_rf ;
	rm -rf code_*.hpp ;

.PHONY: clean
