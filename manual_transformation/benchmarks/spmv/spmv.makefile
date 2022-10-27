BENCHMARK=$(notdir $(shell pwd))

HEARTBEAT_DEFAULT_OPTIONS?=-DCHUNK_LOOP_ITERATIONS
CORRECTNESS_VERSION?=-DHEARTBEAT_BRANCHES

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
# Other
# ======================
$(BENCHMARK)_clean:
	rm -f heartbeat_branches heartbeat_versioning correctness *.json ;

.PHONY: $(BENCHMARK)_clean
