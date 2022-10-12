TASKPARTS?=../../runtime/include
CXXFLAGS=--std=c++17 -fno-stack-protector -DTASKPARTS_POSIX -DTASKPARTS_X64 -DTASKPARTS_TPALRTS_HARDWARE_ALARM_POLLING -DTASKPARTS_STATS -DTASKPARTS_LOG -DTASKPARTS_TPALRTS -fno-asynchronous-unwind-tables -fomit-frame-pointer -g -I $(TASKPARTS)
LIBS=-lpthread -lm -lstdc++

OPTS=-O3 
#OPTS=-O3 -fno-vectorize -fno-slp-vectorize

INPUT=100 100 100

all: manual_transformed_program

manual_transformed_program: test.cpp
	clang++ $(CXXFLAGS) -DORIGINAL_PROGRAM $(LIBS) $^ $(OPTS) -o $@

run: manual_transformed_program
	export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=240000; ./$^ $(INPUT)

clean:
	rm -f manual_transformed_program *.json

.PHONY: clean run
