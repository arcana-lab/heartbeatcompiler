TASKPARTS?=../../../taskparts/include/
LIBS=-lpthread -lm -lstdc++
CXXFLAGS=--std=c++17 -fno-stack-protector -DTASKPARTS_POSIX -DTASKPARTS_X64 -DTASKPARTS_TPALRTS_HARDWARE_ALARM_POLLING -DTASKPARTS_STATS -DTASKPARTS_LOG -DTASKPARTS_TPALRTS -fno-asynchronous-unwind-tables -fomit-frame-pointer -I $(TASKPARTS)
INPUT=100 100000 1 1 1
OPTS=-O3 
#OPTS=-O3 -fno-vectorize -fno-slp-vectorize

all: original_program program_with_heartbeat

# Experiment
# Plot 1:
#  just $ make
# Plot 2:
#  uncomment the second OPTS above and make clean and make

original_program: incr_array.cpp Heartbeats.cpp
	clang++ $(CXXFLAGS) $(LIBS) $^ $(OPTS) -o $@

program_with_heartbeat: program_with_heartbeat.bc Heartbeats.o
	clang $(CXXFLAGS) $(LIBS) $^ $(OPTS) -o $@

%.bc: %.c
	clang $(CXXFLAGS) -O1 -Xclang -disable-llvm-passes -emit-llvm -c $^ -o $@
	llvm-dis $@ 

%.bc: %.cpp
	clang++ $(CXXFLAGS) -O1 -Xclang -disable-llvm-passes -emit-llvm -c $^ -o $@
	llvm-dis $@ 

%.o: %.cpp
	clang $^ -c -O3 -o $@

program_with_heartbeat.bc: incr_array.bc
	noelle-norm $< -o program_normalized.bc
	llvm-dis program_normalized.bc
	noelle-parallel-load -load ~/CAT/lib/HeartBeat.so -heartbeat program_normalized.bc -o $@
	llvm-dis program_with_heartbeat.bc

run: program_with_heartbeat
	export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=240000; ./$^ $(INPUT)

clean:
	rm -f *.bc *.ll program_with_heartbeat original_program *.o *.json a.out

.PHONY: clean run
