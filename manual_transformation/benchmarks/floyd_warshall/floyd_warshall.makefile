DEFAULT_OPTIONS?=-DCHUNK_LOOP_ITERATIONS
CORRECTNESS_VERSION?=-DHEARTBEAT_VERSIONING

# ======================
# Branches
# ======================
heartbeat_branches: heartbeat.cpp
	clang++ $(CXXFLAGS_HEARTBEAT) -DHEARTBEAT_BRANCHES $(DEFAULT_OPTIONS) $(LIBS_HEARTBEAT) $^ $(OPTS) -o $@

run_heartbeat_branches: heartbeat_branches
	export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_FREQUENCY); ./$^

# ======================
# Versioning
# ======================
heartbeat_versioning: heartbeat.cpp
	clang++ $(CXXFLAGS_HEARTBEAT) -DHEARTBEAT_VERSIONING $(DEFAULT_OPTIONS) $(LIBS_HEARTBEAT) $^ $(OPTS) -o $@

run_heartbeat_versioning: heartbeat_versioning
	export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_FREQUENCY); ./$^

# ======================
# Correctness
# ======================
correctness: heartbeat.cpp
	clang++ $(CXXFLAGS_HEARTBEAT) -DTEST_CORRECTNESS $(CORRECTNESS_VERSION) $(DEFAULT_OPTIONS) $(LIBS_HEARTBEAT) $^ $(OPTS) -o $@

run_correctness: correctness
	export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_FREQUENCY); ./$^

# ======================
# Other
# ======================
floyd_warshall_clean:
	rm -f heartbeat_branches heartbeat_versioning correctness *.json ;

.PHONY: floyd_warshall_clean