TASKPARTS?=../../../taskparts/include/
LIBS=-lpthread -lm -lstdc++
CXXFLAGS=--std=c++17 -fno-stack-protector -DTASKPARTS_POSIX -DTASKPARTS_X64 -DTASKPARTS_TPALRTS_HARDWARE_ALARM_POLLING -fno-asynchronous-unwind-tables -fomit-frame-pointer -DTASKPARTS_TPALRTS -I $(TASKPARTS)

all: original_program program_with_heartbeat

original_program: incr_array.cpp Heartbeats.cpp
	clang++ $(CXXFLAGS) $(LIBS) $^ -O3 -o $@

program_with_heartbeat: program_with_heartbeat.bc Heartbeats.o
	clang $(CXXFLAGS) $(LIBS) $^ -O3 -o $@

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

clean:
	rm -f *.bc *.ll program_with_heartbeat original_program *.o

.PHONY: clean
