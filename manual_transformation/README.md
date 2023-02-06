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

To compile a benchmark using the rollforwarding pipeline, add the following option while compiling
run `make heartbeat_versioning ENABLE_ROLLFORWARD=true`
