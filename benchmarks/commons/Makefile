benchmark = $(notdir $(shell pwd))

CC = clang
CXX = clang++

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

# whether to collect and print kernel_exectime when program finishes
# options: true, false
COLLECT_KERNEL_TIME ?= true
ifeq ($(COLLECT_KERNEL_TIME), true)
	MACROS += -DCOLLECT_KERNEL_TIME
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

# ============================================
# Baseline
# ============================================
MACROS_BASELINE += -DUSE_BASELINE $(MACROS)
FLAGS_BASELINE += $(FLAGS)
OPTS_BASELINE += $(OPTS)
LIBS_BASELINE += $(LIBS)

baseline: main.cpp $(benchmark).cpp
	$(CXX) $(MACROS_BASELINE) $(FLAGS_BASELINE) $(OPTS_BASELINE) $^ -o $@ $(LIBS_BASELINE)

run_baseline: baseline
	./$<

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
LIBS_OPENCILK += \
	$(LIBS) \
	$(CUSTOM_MALLOC) # needs to go last, or else it won't be used

opencilk: main.cpp $(benchmark).cpp
	$(OPENCILK_COMPILER) $(MACROS_OPENCILK) $(FLAGS_OPENCILK) $(OPTS_OPENCILK) $^ -o $@ $(LIBS_OPENCILK)

run_opencilk: opencilk
	./$<

# ============================================
# OpenMP
# ============================================
MACROS_OPENMP += -DUSE_OPENMP $(MACROS)
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

openmp: main.cpp $(benchmark).cpp
	$(CXX) $(MACROS_OPENMP) $(FLAGS_OPENMP) $(OPTS_OPENMP) $^ -o $@ $(LIBS_OPENMP)

run_openmp: openmp
	./$<

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
	rm -rf baseline opencilk openmp ;
	rm -rf heartbeat_branches heartbeat_versioning ;
	rm -rf heartbeat_versioning_rollforward.s heartbeat_versioning.rf.s heartbeat_versioning.rf.o rollforward.o heartbeat_versioning_rf ;
	rm -rf code_*.hpp ;

.PHONY: condor clean
