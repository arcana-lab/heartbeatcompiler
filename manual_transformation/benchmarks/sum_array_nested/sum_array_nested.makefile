BENCHMARK=$(notdir $(shell pwd))

HEARTBEAT_DEFAULT_OPTIONS?=-DCHUNK_LOOP_ITERATIONS -DCOLLECT_KERNEL_TIME -DHIGHEST_NESTED_LEVEL=2 -DSMALLEST_GRANULARITY=2
CORRECTNESS_VERSION?=-DHEARTBEAT_VERSIONING

# ======================
# Branches
# ======================
heartbeat_branches: heartbeat.cpp
	$(CXX) -DHEARTBEAT_BRANCHES $(HEARTBEAT_DEFAULT_OPTIONS) $(OPT_FLAGS) $(TASKPARTS_FLAGS) $^ -o $@ $(LINKER_FLAGS)

run_heartbeat_branches: heartbeat_branches
	export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_BASE_FREQUENCY); ./$<

# ======================
# Versioning
# ======================
heartbeat_versioning: heartbeat.cpp
	$(CXX) -DHEARTBEAT_VERSIONING $(HEARTBEAT_DEFAULT_OPTIONS) $(OPT_FLAGS) $(TASKPARTS_FLAGS) $^ -o $@ $(LINKER_FLAGS)

run_heartbeat_versioning: heartbeat_versioning
	export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_BASE_FREQUENCY); ./$<

heartbeat_versioning_optimized: heartbeat.cpp
	$(CXX) -DHEARTBEAT_VERSIONING_OPTIMIZED $(HEARTBEAT_DEFAULT_OPTIONS) $(OPT_FLAGS) $(TASKPARTS_FLAGS) $^ -o $@ $(LINKER_FLAGS)

run_heartbeat_versioning_optimized: heartbeat_versioning_optimized
	export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_BASE_FREQUENCY); ./$<

# ======================
# Correctness
# ======================
correctness: heartbeat.cpp
	$(CXX) -DTEST_CORRECTNESS $(CORRECTNESS_VERSION) $(HEARTBEAT_DEFAULT_OPTIONS) $(OPT_FLAGS) $(TASKPARTS_FLAGS) $^ -o $@ $(LINKER_FLAGS)

run_correctness: correctness
	export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_BASE_FREQUENCY); ./$<

test_correctness: test_correctness.sh
	./$<

# ======================
# Evaluation
# ======================
condor:
	cd eval ; condor_submit pipeline.con ;

# ======================
# Other
# ======================
$(BENCHMARK)_clean:
	rm -f heartbeat_branches heartbeat_versioning heartbeat_versioning_optimized correctness *.json ;
	rm -f eval/condor.* eval/tmp* ;

.PHONY: $(BENCHMARK)_clean
