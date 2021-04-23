OBJECTS=uncompromising_parallelism_compiler_step5.o
INCLUDES=-I.
CPP=clang++

all: crazy sequential

%.o: %.cpp
	$(CPP) -O0 $(INCLUDES) -g -c $^ -o $@

sequential: sequential.o
	$(CPP) $^ -o $@ $(LIBS)

crazy: $(OBJECTS)
	$(CPP) $^ -o $@ $(LIBS)

clean:
	rm -f *.o crazy vgcor* sequential

.PHONY: clean
