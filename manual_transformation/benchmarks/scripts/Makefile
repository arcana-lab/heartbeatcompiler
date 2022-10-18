CXXFLAGS_BASELINE=--std=c++17
LIBS_BASELINE=-lm -lstdc++

CXXFLAGS_OPENCILK=$(CXXFLAGS_BASELINE) -DUSE_OPENCILK -fcilkplus
LIBS_OPENCILK=$(LIBS_BASELINE) -lcilkrts

CXXFLAGS_OPENMP=$(CXXFLAGS_BASELINE) -DUSE_OPENMP -fopenmp
LIBS_OPENMP=$(LIBS_BASELINE) -fopenmp

TASKPARTS?=../../runtime/include
CXXFLAGS=$(CXXFLAGS_BASELINE) -fno-stack-protector -DTASKPARTS_POSIX -DTASKPARTS_X64 -DTASKPARTS_TPALRTS_HARDWARE_ALARM_POLLING -DTASKPARTS_STATS -DTASKPARTS_LOG -DTASKPARTS_TPALRTS -fno-asynchronous-unwind-tables -fomit-frame-pointer -I $(TASKPARTS)
LIBS=$(LIBS_BASELINE) -lpthread

OPTS=-O3 -march=native -fno-vectorize -fno-slp-vectorize

all: baseline

baseline: baseline.cpp
	clang++ $(CXXFLAGS_BASELINE) $(LIBS_BASELINE) $^ $(OPTS) -o $@

correctness: baseline.cpp
	clang++ $(CXXFLAGS_BASELINE) -DTEST_CORRECTNESS $(LIBS_BASELINE) $^ $(OPTS) -o $@

opencilk: opencilk.cpp
	clang++ $(CXXFLAGS_OPENCILK) $(LIBS_OPENCILK) $^ $(OPTS) -o $@

openmp: openmp.cpp
	clang++ $(CXXFLAGS_OPENMP) $(LIBS_OPENMP) $^ $(OPTS) -o $@

clean:
	rm -f baseline correctness opencilk openmp;

.PHONY: clean baseline correctness opencilk openmp