CXXFLAGS_BASELINE=--std=c++17
LIBS_BASELINE=-lm -lstdc++

OPENCILK_COMPILER?=/nfs-scratch/yso0488/opencilk/build/bin/clang++
CXXFLAGS_OPENCILK=$(CXXFLAGS_BASELINE) -DUSE_OPENCILK -fopencilk
LIBS_OPENCILK=$(LIBS_BASELINE)

CXXFLAGS_OPENMP=$(CXXFLAGS_BASELINE) -DUSE_OPENMP -fopenmp
LIBS_OPENMP=$(LIBS_BASELINE) -fopenmp

TASKPARTS?=../../runtime/include
VERSION?=-DHEARTBEAT_BRANCHES
CXXFLAGS_HEARTBEAT=$(CXXFLAGS_BASELINE) -fno-stack-protector -DTASKPARTS_POSIX -DTASKPARTS_X64 -DTASKPARTS_TPALRTS_HARDWARE_ALARM_POLLING -DTASKPARTS_STATS -DTASKPARTS_LOG -DTASKPARTS_TPALRTS -fno-asynchronous-unwind-tables -fomit-frame-pointer -I $(TASKPARTS)
LIBS_HEARTBEAT=$(LIBS_BASELINE) -lpthread

OPTS=-O3 -march=native -fno-vectorize -fno-slp-vectorize

all: baseline

# ======================
# Baseline
# ======================
baseline: baseline.cpp
	clang++ $(CXXFLAGS_BASELINE) $(LIBS_BASELINE) $^ $(OPTS) -o $@

run_baseline: baseline
	./baseline

# ======================
# OpenCilk
# ======================
opencilk: opencilk.cpp
	$(OPENCILK_COMPILER) $(CXXFLAGS_OPENCILK) $(LIBS_OPENCILK) $^ $(OPTS) -o $@

run_opencilk: opencilk
	./opencilk

# ======================
# OpenMP
# ======================
openmp: openmp.cpp
	clang++ $(CXXFLAGS_OPENMP) $(LIBS_OPENMP) $^ $(OPTS) -o $@

run_openmp: openmp
	./openmp

# ======================
# Heartbeat
# ======================
heartbeat_branches: heartbeat.cpp
	clang++ $(CXXFLAGS_HEARTBEAT) -DHEARTBEAT_BRANCHES $(LIBS_HEARTBEAT) $^ $(OPTS) -o $@

run_heartbeat_branches: heartbeat_branches
	export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=240000; ./heartbeat_branches

heartbeat_versioning: heartbeat.cpp
	clang++ $(CXXFLAGS_HEARTBEAT) -DHEARTBEAT_VERSIONING $(LIBS_HEARTBEAT) $^ $(OPTS) -o $@

run_heartbeat_versioning: heartbeat_versioning
	export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=240000; ./heartbeat_versioning

correctness: heartbeat.cpp
	clang++ $(CXXFLAGS_HEARTBEAT) $(VERSION) -DTEST_CORRECTNESS $(LIBS_HEARTBEAT) $^ $(OPTS) -o $@

run_correctness: correctness
	export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=240000; ./correctness

# ======================
# Other
# ======================
clean:
	rm -f baseline opencilk openmp heartbeat_branches heartbeat_versioning correctness *.json;

.PHONY: clean
