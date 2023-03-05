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
# options: true, false
ENABLE_ROLLFORWARD ?= false
ifeq ($(ENABLE_ROLLFORWARD), true)
	CXX = g++
	MACROS += -DENABLE_ROLLFORWARD
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
ifeq ($(ENABLE_ROLLFORWARD), false)
	TASKPARTS_FLAGS += -DTASKPARTS_TPALRTS_HARDWARE_ALARM_POLLING
endif
MACROS += $(TASKPARTS_FLAGS)

FLAGS = \
	--std=c++17 \
	-I $(TASKPARTS_INCLUDE)

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
-include Makefile_heartbeat

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
	rm -rf baseline opencilk openmp heartbeat ;
	rm -rf *.bc *.ll ;
	rm -rf heartbeat_versioning_rollforward.s heartbeat_versioning.rf.s heartbeat_versioning.rf.o rollforward.o heartbeat_versioning_rf ;
	rm -rf code_*.hpp ;

.PHONY: condor clean
