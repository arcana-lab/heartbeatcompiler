MACROS_HEARTBEAT_MANUAL += -DUSE_HB_MANUAL $(MACROS_HEARTBEAT)
FLAGS_HEARTBEAT_MANUAL += $(FLAGS_HEARTBEAT)
OPTS_HEARTBEAT_MANUAL += $(OPTS_HEARTBEAT)
LIBS_HEARTBEAT_MANUAL += $(LIBS_HEARTBEAT)

# ============================================
# Heartbeat Manual
# ============================================
%_hbm.bc: %.cpp
	$(CXX) $(MACROS_HEARTBEAT_MANUAL) $(FLAGS_HEARTBEAT_MANUAL) -c $< -o $@

heartbeat_manual_normalized.bc: main_hbm.bc heartbeat_manual_hbm.bc
	llvm-link $^ -o $@
	noelle-norm $@ -o $@

hbm: heartbeat_manual
hb_manual: heartbeat_manual
heartbeat_manual: heartbeat_manual_normalized.bc promotion_handler_hbm.bc
ifeq ($(ENABLE_ROLLFORWARD), true)
	make heartbeat_manual_rf
	cp heartbeat_manual_rf $@
else
	llvm-link $^ -o $@.bc
	$(CXX) $(OPTS_HEARTBEAT_MANUAL) $@.bc -o $@ $(LIBS_HEARTBEAT_MANUAL)
endif

run_hbm: run_heartbeat_manual
run_hb_manual: run_heartbeat_manual
run_heartbeat_manual: heartbeat_manual
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
# Heartbeat Manual (Rollforward)
# ============================================
heartbeat_manual.s: heartbeat_manual_normalized.bc
	$(CXX) $(OPTS_HEARTBEAT_MANUAL) $< -S -o $@

heartbeat_manual.rf.s: heartbeat_manual.s
	ROLLFORWARD_PATH=$(ROLLFORWARD_PATH) $(ROLLFORWARD_COMPILER) $< $@

heartbeat_manual.rf.o: heartbeat_manual.rf.s
	$(CXX) -I $(ROLLFORWARD_INCLUDE) $< -c -o $@

# resolve the "undefined reference to __rf_signal" error
heartbeat_manual.rf.ld.o: heartbeat_manual.rf.o
	ld -r --defsym=__rf_signal=0x42 $< -o $@

rollforward_manual.o: $(ROLLFORWARD_INCLUDE)/rollforward.c
	$(CXX) $< -c -o $@

heartbeat_manual_rf: heartbeat_manual.rf.ld.o rollforward_manual.o bench_hbm.bc promotion_handler_hbm.bc
	$(CXX) $(OPTS_HEARTBEAT_MANUAL) $^ -o $@ $(LIBS_HEARTBEAT_MANUAL)
