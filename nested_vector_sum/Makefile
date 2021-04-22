OBJECTS=uncompromising_parallelism_compiler_step5.o
INCLUDES=-I.
CPP=clang++

all: crazy

%.o: %.cpp
	$(CPP) $(INCLUDES) -g -c $^ -o $@

crazy: $(OBJECTS)
	$(CPP) -pthread $^ -o $@ $(LIBS)

clean:
	rm -f *.o crazy

.PHONY: clean
