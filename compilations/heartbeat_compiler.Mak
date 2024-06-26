MACROS_HEARTBEAT_COMPILER += -DUSE_HB_COMPILER $(MACROS_HEARTBEAT)
FLAGS_HEARTBEAT_COMPILER += $(FLAGS_HEARTBEAT)
OPTS_HEARTBEAT_COMPILER += $(OPTS_HEARTBEAT)
LIBS_HEARTBEAT_COMPILER += $(LIBS_HEARTBEAT)

# ============================================
# Heartbeat Compiler
# ============================================
%_hbc.bc: %.cpp
	$(CXX) $(MACROS_HEARTBEAT_COMPILER) $(FLAGS_HEARTBEAT_COMPILER) -c $< -o $@

heartbeat_compiler_normalized.bc: main_hbc.bc
	noelle-norm $^ -o $@

heartbeat_compiler.bc: heartbeat_compiler_normalized.bc
ifeq ($(ENABLE_ROLLFORWARD), true)
	make rf_test.bc
	llvm-link rf_test.bc $< -o $<
endif
	noelle-parallel-load -noelle-verbose=2 -load $(ROOT_DIR)/build/lib/Heartbeat.so -heartbeat $(PASS_ARGUMENTS_HEARTBEAT_COMPILER) $^ -o $@

hbc: heartbeat_compiler
hb_compiler: heartbeat_compiler
heartbeat_compiler: heartbeat_compiler.bc promotion_handler_hbc.bc
ifeq ($(ENABLE_ROLLFORWARD), true)
	make heartbeat_compiler_rf
	cp heartbeat_compiler_rf $@
else
	llvm-link $^ -o $@.bc
	$(CXX) $(OPTS_HEARTBEAT_COMPILER) $@.bc -o $@ $(LIBS_HEARTBEAT_COMPILER)
endif

run_hbc: run_heartbeat_compiler
run_hb_compiler: run_heartbeat_compiler
run_heartbeat_compiler: heartbeat_compiler
ifneq ($(WORKERS), 0)
	TASKPARTS_NUM_WORKERS=$(WORKERS) \
	TASKPARTS_BENCHMARK_WARMUP_SECS=$(WARMUP_SECS) \
	TASKPARTS_BENCHMARK_NUM_REPEAT=$(NUM_REPEAT) \
	TASKPARTS_BENCHMARK_VERBOSE=$(VERBOSE) \
	TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_FREQUENCY_KHZ) \
	TASKPARTS_KAPPA_USEC=$(KAPPA_USECS) \
	./$< $(ARGS)
else
	TASKPARTS_BENCHMARK_WARMUP_SECS=$(WARMUP_SECS) \
	TASKPARTS_BENCHMARK_NUM_REPEAT=$(NUM_REPEAT) \
	TASKPARTS_BENCHMARK_VERBOSE=$(VERBOSE) \
	TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_FREQUENCY_KHZ) \
	TASKPARTS_KAPPA_USEC=$(KAPPA_USECS) \
	./$< $(ARGS)
endif

# ============================================
# Heartbeat Compiler (Rollforward)
# ============================================
rf_test.bc: rf_test.cpp
	$(CXX) -O3 -emit-llvm -c $< -o $@

heartbeat_compiler.s: heartbeat_compiler.bc
	$(CXX) $(OPTS_HEARTBEAT_COMPILER) $< -S -o $@

heartbeat_compiler.rf.s: heartbeat_compiler.s
	ROLLFORWARD_PATH=$(ROLLFORWARD_PATH) $(ROLLFORWARD_COMPILER) $< $@

heartbeat_compiler.rf.o: heartbeat_compiler.rf.s
	$(CXX) -I $(ROLLFORWARD_INCLUDE) $< -c -o $@

# resolve the "undefined reference to __rf_signal" error
heartbeat_compiler.rf.ld.o: heartbeat_compiler.rf.o
	ld -r --defsym=__rf_signal=0x42 $< -o $@

rollforward_compiler.o: $(ROLLFORWARD_INCLUDE)/rollforward.c
	$(CXX) $< -c -o $@

heartbeat_compiler_rf: heartbeat_compiler.rf.ld.o rollforward_compiler.o bench_hbm.bc promotion_handler_hbm.bc
	$(CXX) $(OPTS_HEARTBEAT_COMPILER) $^ -o $@ $(LIBS_HEARTBEAT_COMPILER)
