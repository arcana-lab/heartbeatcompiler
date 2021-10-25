LIBS=-lpthread -lm -lstdc++

all: original_program program_with_heartbeat

original_program: incr_array.cpp
	clang++ $^ -O3 -o $@

program_with_heartbeat: program_with_heartbeat.bc Heartbeats.o
	clang $(LIBS) $^ -O3 -o $@

%.bc: %.c
	clang -O1 -Xclang -disable-llvm-passes -emit-llvm -c $^ -o $@
	llvm-dis $@ 

%.bc: %.cpp
	clang++ -O1 -Xclang -disable-llvm-passes -emit-llvm -c $^ -o $@
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
