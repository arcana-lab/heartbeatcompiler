CXXFLAGS_BASELINE=--std=c++17
LIBS_BASELINE=-lm -lstdc++

OPENCILK_COMPILER?=/nfs-scratch/yso0488/opencilk/build/bin/bin/opencilk++
CXXFLAGS_OPENCILK=$(CXXFLAGS_BASELINE) -DUSE_OPENCILK -fopencilk
LIBS_OPENCILK=$(LIBS_BASELINE)

CXXFLAGS_OPENMP=$(CXXFLAGS_BASELINE) -DUSE_OPENMP -fopenmp
LIBS_OPENMP=$(LIBS_BASELINE) -fopenmp

OPTS=-O3 -march=native -fno-vectorize -fno-slp-vectorize

all: baseline

# ======================
# Baseline
# ======================
baseline: baseline.cpp
	clang++ $(CXXFLAGS_BASELINE) $(LIBS_BASELINE) $^ $(OPTS) -o $@

run_baseline: baseline
	./$^

# ======================
# OpenCilk
# ======================
opencilk: opencilk.cpp
	$(OPENCILK_COMPILER) $(CXXFLAGS_OPENCILK) $(LIBS_OPENCILK) $^ $(OPTS) -o $@

run_opencilk: opencilk
	./$^

# ======================
# OpenMP
# ======================
openmp: openmp.cpp
	clang++ $(CXXFLAGS_OPENMP) $(LIBS_OPENMP) $^ $(OPTS) -o $@

run_openmp: openmp
	./$^

# ======================
# Heartbeat
# ======================
TASKPARTS?=../../runtime/include
CPU_FREQUENCY?=3300000
CXXFLAGS_HEARTBEAT=$(CXXFLAGS_BASELINE) -fno-stack-protector -DTASKPARTS_POSIX -DTASKPARTS_X64 -DTASKPARTS_TPALRTS_HARDWARE_ALARM_POLLING -DTASKPARTS_STATS -DTASKPARTS_LOG -DTASKPARTS_TPALRTS -fno-asynchronous-unwind-tables -fomit-frame-pointer -I $(TASKPARTS)
LIBS_HEARTBEAT=$(LIBS_BASELINE) -lpthread

benchmark = $(notdir $(shell pwd))
include ${benchmark}.makefile

# ======================
# Other
# ======================
clean:
	rm -f baseline opencilk openmp ;
	make ${benchmark}_clean ;

.PHONY: clean
