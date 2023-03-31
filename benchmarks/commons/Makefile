ROOT_DIR = $(shell git rev-parse --show-toplevel)

CC = clang
CXX = clang++

# size of input, either benchmarking, tpal or testing
# options: benchmarking, tpal, testing
INPUT_SIZE ?= testing
ifeq ($(INPUT_SIZE), benchmarking)
	MACROS += -DINPUT_BENCHMARKING
else ifeq ($(INPUT_SIZE), tpal)
	MACROS += -DINPUT_TPAL
else ifeq ($(INPUT_SIZE), testing)
	MACROS += -DINPUT_TESTING
endif

# whether to test the correctness of any implementation
# options: true, false
TEST_CORRECTNESS ?= false
ifeq ($(TEST_CORRECTNESS), true)
	MACROS += -DTEST_CORRECTNESS
endif

# whether to enable the rollforward compilation
# and whether to use the heartbeat linux module
# options: true, false
ENABLE_ROLLFORWARD ?= true
USE_HB_KMOD ?= false
ifeq ($(ENABLE_ROLLFORWARD), true)
	MACROS_HEARTBEAT += -DENABLE_ROLLFORWARD
	PASS_ARGUMENTS_HEARTBEAT_COMPILER += -enable_rollforward
else
	MACROS += -DDISABLE_ROLLFORWARD
endif

# include benchmark specific settings
-include bench.config

# taskparts definitions
CPU_BASE_FREQUENCY_KHZ ?= 2800000
TASKPARTS_PATH = $(ROOT_DIR)/runtime
TASKPARTS_INCLUDE = $(TASKPARTS_PATH)/include

# macros used for taskparts runtime
TASKPARTS_PLATFORM ?= -DTASKPARTS_POSIX -DTASKPARTS_X64
TASKPARTS_FLAGS = \
	$(TASKPARTS_PLATFORM) \
	-DTASKPARTS_TPALRTS \
	-DTASKPARTS_STATS
ifeq ($(ENABLE_ROLLFORWARD), true)
	ifeq ($(USE_HB_KMOD), true)
  	TASKPARTS_FLAGS += -DTASKPARTS_TPALRTS_HBTIMER_KMOD
	endif
else
	TASKPARTS_FLAGS += -DTASKPARTS_TPALRTS_HARDWARE_ALARM_POLLING
endif
MACROS += $(TASKPARTS_FLAGS)

FLAGS = \
	--std=c++17 \
	-I$(TASKPARTS_INCLUDE)

OPTS = \
	-O3 -march=native \
	-fno-stack-protector \
	-fno-asynchronous-unwind-tables \
	-fomit-frame-pointer
ifeq ($(CXX), clang++)
	OPTS += -fno-vectorize -fno-slp-vectorize
else ifeq ($(CXX), g++)
	OPTS += -fno-tree-vectorize
endif

LIBS = -lm -lstdc++ -pthread -ldl
ifeq (, $(shell which jemalloc-config))
	CUSTOM_MALLOC =
else
	JEMALLOCLD = $(shell jemalloc-config --libdir)
	CUSTOM_MALLOC = -L$(JEMALLOCLD) -Wl,-rpath,$(JEMALLOCLD) -ljemalloc
	LIBS += $(CUSTOM_MALLOC)
endif

# ============================================
# Baseline
# ============================================
MACROS_BASELINE += -DUSE_BASELINE $(MACROS)
FLAGS_BASELINE += $(FLAGS)
OPTS_BASELINE += $(OPTS)
LIBS_BASELINE += $(LIBS)

baseline: main.cpp bench.cpp
	$(CXX) $(MACROS_BASELINE) $(FLAGS_BASELINE) $(OPTS_BASELINE) $^ -o $@ $(LIBS_BASELINE)

run_baseline: baseline
	TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_BASE_FREQUENCY_KHZ) ./$<

# ============================================
# OpenCilk
# ============================================
OPENCILK_COMPILER ?= /nfs-scratch/yso0488/opencilk/build/bin/bin/opencilk++

MACROS_OPENCILK += -DUSE_OPENCILK $(MACROS)
FLAGS_OPENCILK += $(FLAGS) -fopencilk
OPTS_OPENCILK += $(OPTS)
ifneq ($(CXX), clang++)
	OPTS_OPENCILK += -fno-vectorize -fno-slp-vectorize
endif
LIBS_OPENCILK += $(LIBS)

opencilk: main.cpp bench.cpp
	$(OPENCILK_COMPILER) $(MACROS_OPENCILK) $(FLAGS_OPENCILK) $(OPTS_OPENCILK) $^ -o $@ $(LIBS_OPENCILK)

run_opencilk: opencilk
	TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_BASE_FREQUENCY_KHZ) ./$<

# ============================================
# OpenMP
# ============================================
MACROS_OPENMP += -DUSE_OPENMP $(MACROS)
FLAGS_OPENMP += $(FLAGS) -fopenmp
OPTS_OPENMP += $(OPTS)
LIBS_OPENMP += $(LIBS) -fopenmp

openmp: main.cpp bench.cpp
	$(CXX) $(MACROS_OPENMP) $(FLAGS_OPENMP) $(OPTS_OPENMP) $^ -o $@ $(LIBS_OPENMP)

run_openmp: openmp
	TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_BASE_FREQUENCY_KHZ) ./$<

# ============================================
# Heartbeat
# ============================================
HEARTBEAT_LINUX = $(ROOT_DIR)/heartbeat-linux
ROLLFORWARD_PATH = $(ROOT_DIR)/rollforward/src
ROLLFORWARD_INCLUDE = $(ROLLFORWARD_PATH)/x64
ROLLFORWARDXX_PATH = $(TASKPARTS_PATH)/benchmark/tpal
ROLLFORWARDXX = $(ROLLFORWARDXX_PATH)/rollforward++

MACROS_HEARTBEAT += $(MACROS)

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
	PASS_ARGUMENTS_HEARTBEAT_COMPILER += -chunk_loop_iterations
endif

# include benchmark heartbeat specific settings
-include heartbeat.config

FLAGS_HEARTBEAT += $(FLAGS)
ifeq ($(ENABLE_ROLLFORWARD), true)
	FLAGS_HEARTBEAT += -I$(ROLLFORWARD_INCLUDE)
	ifeq ($(USE_HB_KMOD), true)
		FLAGS_HEARTBEAT += -I$(HEARTBEAT_LINUX)/user
	endif
endif

OPTS_HEARTBEAT += $(OPTS)
ifeq ($(ENABLE_ROLLFORWARD), true)
	ifeq ($(USE_HB_KMOD), true)
		LIBS_HEARTBEAT += -L$(HEARTBEAT_LINUX) -lhb
	endif
endif
LIBS_HEARTBEAT += $(LIBS)

-include Makefile_heartbeat_manual
-include Makefile_heartbeat_compiler

# ============================================
# Evaluation
# ============================================
condor:
	cd condor ; condor_submit job.con ;

condor_clean:
	cd condor ; rm -rf condor.* ;

# ============================================
# Other
# ============================================
clean:
	rm -rf baseline opencilk openmp ;
	rm -rf heartbeat_manual heartbeat_manual_rf ;
	rm -rf heartbeat_compiler heartbeat_compiler_rf ;
	rm -rf *.s *.o ;
	rm -rf *.bc *.ll ;

.PHONY: condor clean
