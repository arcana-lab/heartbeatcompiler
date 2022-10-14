TASKPARTS?=../../runtime/include
CXXFLAGS=--std=c++17 -fno-stack-protector -DTASKPARTS_POSIX -DTASKPARTS_X64 -DTASKPARTS_TPALRTS_HARDWARE_ALARM_POLLING -DTASKPARTS_STATS -DTASKPARTS_LOG -DTASKPARTS_TPALRTS -fno-asynchronous-unwind-tables -fomit-frame-pointer -g -I $(TASKPARTS)
LIBS=-lpthread -lm -lstdc++

OPTS=-O3 -fno-vectorize -fno-slp-vectorize

all: baseline 

baseline: baseline.cpp
	clang++ $(CXXFLAGS) $(LIBS) $^ $(OPTS) -o $@

clean:
	rm -f baseline ;

.PHONY: clean
