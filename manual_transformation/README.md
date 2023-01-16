# Setup
run `make` ; `cd benchmarks ; make`

# Compile a benchmark
run `make heartbeat_versioning [OPTIONS]`

Under the "Heartbeat" section of Makefile lists compilation options and their default values

For example, to use test input and enable further splitting on leftover task:
run `make heartbeat_versioning INPUT_SIZE=testing LEFTOVER_SPLITTABLE=true`

By defult, heartbeat transformation chunks loop iterations, the default chunk size can be found under `$(bench).config`
However, to supply customized chunksize, depending on the number of levels of nested loop
run `make heartbeat_versioning CHUNKSIZE_0=... CHUNKSIZE_1=... ...`

Currently, rollforward transformation has only been done on `floyd_warshall`, and will be compiled with `g++`, to compile
run `make heartbeat_versioning ENABLE_ROLLFORWARD`
