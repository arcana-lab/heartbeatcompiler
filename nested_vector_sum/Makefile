OBJECTS=uncompromising_parallelism_compiler_step5.o
FJ_OBJECTS=uncompromising_parallelism_compiler_step5_forkjoin.o
INCLUDES=-I.
CPP=clang++

all: crazy sequential

%.o: %.cpp
	$(CPP) $(INCLUDES) -g3 -c $^ -o $@

sequential: sequential.o
	$(CPP) $^ -o $@ $(LIBS)

parallel:
	$(CPP) -std=c++17 -pthread -I. -I $(MCSL_PATH) -I $(CMDLINE_PATH) parallel.cpp -o $@ $(LIBS)

crazy: $(OBJECTS)
	$(CPP) $^ -o $@ $(LIBS)

forkjoin: $(FJ_OBJECTS)
	$(CPP) $^ -o $@ $(LIBS)

clean:
	rm -f *.o crazy vgcor* sequential parallel

.PHONY: clean
