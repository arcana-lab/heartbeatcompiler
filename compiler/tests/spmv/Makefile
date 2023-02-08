CC = clang
CXX = clang++

FLAGS = --std=c++17
OPTS = -O3 -march=native -fno-vectorize -fno-slp-vectorize
LIBS = -lm -lstdc++

INPUT = 100 100

# ===
# spmv-specific logic
# ===
# input class used for spmv
# options: SPMV_{RANDOM, POWERLAW, ARROWHEAD, DENSE, DIAGONAL, NORMAL}
INPUT_CLASS ?= SPMV_RANDOM
ifeq ($(INPUT_CLASS), SPMV_RANDOM)
  MACROS += -DSPMV_RANDOM
else ifeq ($(INPUT_CLASS), SPMV_POWERLAW)
  MACROS += -DSPMV_POWERLAW
else ifeq ($(INPUT_CLASS), SPMV_ARROWHEAD)
  MACROS += -DSPMV_ARROWHEAD
else ifeq ($(INPUT_CLASS), SPMV_DENSE)
  MACROS += -DSPMV_DENSE
else ifeq ($(INPUT_CLASS), SPMV_DIAGONAL)
  MACROS += -DSPMV_DIAGONAL
else ifeq ($(INPUT_CLASS), SPMV_NORMAL)
  MACROS += -DSPMV_NORMAL
endif

MACROS += -DINPUT_BENCHMARKING -DTEST_CORRECTNESS

# ======================
# Baseline
# ======================
MACROS_BASELINE += $(MACROS) -DUSE_BASELINE
FLAGS_BASELINE += $(FLAGS)
OPTS_BASELINE += $(OPTS)
LIBS_BASELINE += $(LIBS)

baseline: test.cpp
	$(CXX) $(MACROS_BASELINE) $(FLAGS_BASELINE) $(OPTS_BASELINE) $^ -o $@ $(LIBS_BASELINE)

run_baseline: baseline
	./$^

# ======================
# Heartbeat
# ======================
TASKPARTS_PATH ?= ../../../manual_transformation/runtime
TASKPARTS_INCLUDE_PATH = $(TASKPARTS_PATH)/include
TASKPARTS_INCLUDES = $(TASKPARTS_INCLUDE_PATH)

MACROS_HEARTBEAT += $(MACROS) -DUSE_HEARTBEAT \
										-DTASKPARTS_POSIX \
										-DTASKPARTS_X64 \
										-DTASKPARTS_TPALRTS_HARDWARE_ALARM_POLLING \
										-DTASKPARTS_STATS \
										-DTASKPARTS_TPALRTS

FLAGS_HEARTBEAT += $(FLAGS) \
									 -O1 -Xclang -disable-llvm-passes -emit-llvm \
									 -fno-stack-protector -fno-asynchronous-unwind-tables -fomit-frame-pointer \
									 -I $(TASKPARTS_INCLUDES)

OPTS_HEARTBEAT += $(OPTS) \
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

heartbeat: heartbeat.bc
	$(CXX) $(OPTS_HEARTBEAT) $^ -o $@ $(LIBS_HEARTBEAT)

heartbeat.bc: test_normalized.bc
	noelle-parallel-load -noelle-verbose=2 -load ~/CAT/lib/HeartBeat.so -heartbeat $^ -o $@
	llvm-dis $@

test_normalized.bc: test.bc
	noelle-norm $^ -o $@
	llvm-dis $@

test.bc: test.cpp
	./generate_code.sh `cat level.config` `cat live_out.config`
	$(CXX) $(MACROS_HEARTBEAT) $(FLAGS_HEARTBEAT) -c $^ -o $@
	llvm-dis $@

run_heartbeat: heartbeat
	export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=2800000 ; ./$^

clean:
	rm -rf baseline ;
	rm -rf test.bc test_normalized.bc heartbeat.bc *.ll ;
	rm -rf heartbeat ;
	rm -rf code_* ;
