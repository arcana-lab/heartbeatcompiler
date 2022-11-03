CC=clang
CXX=clang++

OPTS=-O3 -march=native -fno-vectorize -fno-slp-vectorize

CXXFLAGS_BASELINE=--std=c++17 -DCOLLECT_KERNEL_TIME
LIBS_BASELINE=-lm -lstdc++

OPENCILK_COMPILER?=/nfs-scratch/yso0488/opencilk/build/bin/bin/opencilk++
CXXFLAGS_OPENCILK=$(CXXFLAGS_BASELINE) -DUSE_OPENCILK -fopencilk
LIBS_OPENCILK=$(LIBS_BASELINE)

CXXFLAGS_OPENMP=$(CXXFLAGS_BASELINE) -DUSE_OPENMP -fopenmp
LIBS_OPENMP=$(LIBS_BASELINE) -fopenmp

all: baseline

# ======================
# Baseline
# ======================
baseline: baseline.cpp
	$(CXX) $(CXXFLAGS_BASELINE) $(OPTS) $^ -o $@ $(LIBS_BASELINE)

run_baseline: baseline
	./$<

# ======================
# OpenCilk
# ======================
opencilk: opencilk.cpp
	$(OPENCILK_COMPILER) $(CXXFLAGS_OPENCILK) $(OPTS) $^ -o $@ $(LIBS_OPENCILK)

run_opencilk: opencilk
	./$<

# ======================
# OpenMP
# ======================
openmp: openmp.cpp
	$(CXX) $(CXXFLAGS_OPENMP) $(OPTS) $^ -o $@ $(LIBS_OPENMP)

run_openmp: openmp
	./$<

openmp_dynamic: openmp.cpp
	$(CXX) $(CXXFLAGS_OPENMP) -DOMP_DYNAMIC $(OPTS) $^ -o $@ $(LIBS_OPENMP)

run_openmp_dynamic: openmp_dynamic
	./$<

openmp_guided: openmp.cpp
	$(CXX) $(CXXFLAGS_OPENMP) -DOMP_GUIDED $(OPTS) $^ -o $@ $(LIBS_OPENMP)

run_openmp_guided: openmp_guided
	./$<

# ======================
# Heartbeat
# ======================
TASKPARTS_PATH?=../../runtime
TASKPARTS_INCLUDE_PATH=$(TASKPARTS_PATH)/include
TASKPARTS_PLATFORM?=-DTASKPARTS_POSIX -DTASKPARTS_X64

INCLUDE_PATHS=\
	-I $(TASKPARTS_INCLUDE_PATH) \
	$(HWLOC_INCLUDE_PATH)

CXX_FLAGS=\
	$(INCLUDE_PATHS) \
	-std=c++17 \
	-fno-stack-protector \

OPT_FLAGS=\
	$(CXX_FLAGS) \
	-O3 \
	-march=native \
	-fno-vectorize \
	-fno-slp-vectorize \
	-fno-asynchronous-unwind-tables \
	-fomit-frame-pointer \

TASKPARTS_FLAGS=\
	$(TASKPARTS_PLATFORM) \
	-DTASKPARTS_TPALRTS \
	-DTASKPARTS_TPALRTS_HARDWARE_ALARM_POLLING \
	-DTASKPARTS_STATS \
	# -DTASKPARTS_LOG

ifeq (, $(shell which jemalloc-config))
CUSTOM_MALLOC =
else
JEMALLOCLD = $(shell jemalloc-config --libdir)
JEMALLOCLS = $(shell jemalloc-config --libs)
CUSTOM_MALLOC = -L$(JEMALLOCLD) -Wl,-rpath,$(JEMALLOCLD) $(JEMALLOCLS) -ljemalloc
endif

LINKER_FLAGS=\
	-pthread \
	$(HWLOC_LIBRARY_PATH) \
	$(CUSTOM_MALLOC) # needs to go last, or else it won't be used

CPU_BASE_FREQUENCY?=3300000

benchmark = $(notdir $(shell pwd))
${benchmark}_clean:

-include ${benchmark}.makefile

# ======================
# Other
# ======================
clean:
	rm -f baseline opencilk openmp openmp_dynamic openmp_guided;
	make ${benchmark}_clean ;

.PHONY: clean

