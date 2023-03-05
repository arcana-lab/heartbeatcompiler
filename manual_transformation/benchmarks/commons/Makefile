ROLLFORWARD_PATH = $(ROOT_DIR)/rollforward/src
ROLLFORWARD_INCLUDE = $(ROLLFORWARD_PATH)/x64
ROLLFORWARDXX_PATH = $(TASKPARTS_PATH)/benchmark/tpal
ROLLFORWARDXX = $(ROLLFORWARDXX_PATH)/rollforward++

MACROS_HEARTBEAT += -DUSE_HB_MANUAL $(MACROS)

# whether to enable heartbeat promotion
# options: true, false
ENABLE_HEARTBEAT_PROMOTION ?= true
ifeq ($(ENABLE_HEARTBEAT_PROMOTION), true)
	MACROS_HEARTBEAT += -DENABLE_HEARTBEAT_PROMOTION
else
	MACROS_HEARTBEAT += -DDISABLE_HEARTBEAT_PROMOTION
endif

# minimum remaining iteration size for heartbeat promotion
# options: unsigned integer >= 2
SMALLEST_GRANULARITY ?= 2
MACROS_HEARTBEAT += -DSMALLEST_GRANULARITY=$(SMALLEST_GRANULARITY)

# whether to chunk loop iterations before calling loop_handler
# options: true, false
CHUNK_LOOP_ITERATIONS ?= true
ifeq ($(CHUNK_LOOP_ITERATIONS), true)
	MACROS_HEARTBEAT += -DCHUNK_LOOP_ITERATIONS
endif

# include benchmark heartbeat specific settings
-include heartbeat.config

FLAGS_HEARTBEAT += $(FLAGS)
ifeq ($(ENABLE_ROLLFORWARD), true)
	FLAGS_HEARTBEAT += -I $(ROLLFORWARD_INCLUDE)
endif

OPTS_HEARTBEAT += $(OPTS)
LIBS_HEARTBEAT += $(LIBS)

# ============================================
# Heartbeat
# ============================================
heartbeat: main.cpp bench.cpp heartbeat_manual.cpp loop_handler.cpp
ifeq ($(ENABLE_ROLLFORWARD), true)
	make heartbeat_rf
	cp heartbeat_rf $@
else
	$(CXX) $(MACROS_HEARTBEAT) $(FLAGS_HEARTBEAT) $(OPTS_HEARTBEAT) $^ -o $@ $(LIBS_HEARTBEAT)
endif

run_heartbeat: heartbeat
	TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_BASE_FREQUENCY_KHZ) ./$<

# ============================================
# Heartbeat (Rollforward)
# ============================================
heartbeat_manual.s: heartbeat_manual.cpp
	$(CXX) $(MACROS_HEARTBEAT) $(FLAGS_HEARTBEAT) $(OPTS_HEARTBEAT) $< -S -o $@

heartbeat_manual.rf.s: heartbeat_manual.s
	ROLLFORWARD_PATH=$(ROLLFORWARD_PATH) $(ROLLFORWARDXX) $< $@

heartbeat_manual.rf.o: heartbeat_manual.rf.s
	$(CXX) -I $(ROLLFORWARD_INCLUDE) $< -c -o $@

rollforward.o: $(ROLLFORWARD_INCLUDE)/rollforward.c
	$(CXX) $< -c -o $@

heartbeat_rf: main.cpp bench.cpp heartbeat_manual.rf.o rollforward.o loop_handler.cpp
	$(CXX) $(MACROS_HEARTBEAT) $(FLAGS_HEARTBEAT) $(OPTS_HEARTBEAT) $^ -o $@ $(LIBS_HEARTBEAT)
