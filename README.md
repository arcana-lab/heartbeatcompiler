# Heartbeat Compiler

## Dependency

The following dependence is required when building [NOELLE](https://github.com/arcana-lab/noelle).
* [LLVM 9.0.1](https://releases.llvm.org/download.html)


## Building

From the root directory, run `make ; source noelle/enable`


## Compiling a benchmark

All benchmarks is under the folder `benchmarks`.

Run `make hbm` will compile the manually transformed benchmark using `gcc` or `clang`.

Run `make hbc` will compile the benchmark using the heartbeat compiler.


## Compiling options

Several options can be applied during the compilation stage to control how the final binary behaves

### General options
- `INPUT_SIZE`: select the input size used by the benchmark, available options are `benchmarking`, `tpal` and `testing`. The default is `testing`.

- `TEST_CORRECTNESS`: this enables running a serial version of the benchmark in the end and compare the result against any implemenation and report the result is CORRECT or INCORRECT. The default is `false`.

### Heartbeat-specific options
- `ENABLE_HEARTBEAT_PROMOTION`: decide whether to enable heartbeat promotion. loop_handler will return 0 immediately if this option is disabled. The default is `true`.

- `ENABLE_ROLLFORWARD`: this controls wether or not to compile the benchmark using the rollforwarding compiler. If disabled, the program will use software polling to determine whether a heartbeat event arrives or not. The default is `true`.

- `CHUNK_LOOP_ITERATIONS`: whether to execute the loop in chunk before calling loop_handler. The default is `true`.

- `SMALLE_GRANULARITY`: the minimal remaining iteration size for a loop to enable heartbeat promotion. The default is `2`.

### Chunksize options
The chunksize option can be supplied in a per-loop-level manner such as `CHUNKSIZE_0` for level 0, `CHUNKSIZE_1` for level 1. Depends on the number of levels a loop nest has. If not supplied, the compilation uses the default chunksize provided in `heartbeat.config` per benchmark folder.

### Benchmark-specific options
The following benchmarks have specific compilation options
- `spmv`:
  - `INPUT_CLASS` could be one of the options in `SPMV_{RANDOM, POWERLAW, ARROWHEAD, DENSE, DIAGONAL, NORMAL}`. The default is `SPMV_RANDOM`.
  - `OPENCILK_IMPL`: what opencilk implementation used for the program, could be one of the options in `OPENCILK_{REDUCER, MANUAL_GRANULARITY, OUTER_FOR, HEURISTICS}`. The defualt is `OPENCILK_HEURISTICS`.

- `floyd_warshall`:
  - `INPUT_CLASS` could be one of the options in `FW_{1K, 2K}`. The default is `FW_1K`.


## Example

Suppose one wants to compile the `plus_reduce_array` using TPAL's input size and verify the correctness of the implementation with no chunking execution:

Run `cd plus_reduce_array ; INPUT_SIZE=tpal TEST_CORRECTNESS=true CHUNK_LOOP_ITERATIONS=false make hbc`

Suppose one wants to compile the `spmv` using TPAL's input size and the arrowhead input class, plus wants to experiment different chunksizes with the software polling mechanism:

Run `cd spmv ; INPUT_SIZE=tpal INPUT_CLASS=SPMV_ARROWHEAD CHUNKSIZE_0=2 CHUNKSIZE_1=2048 ENABLE_ROLLFORWARD=false make hbc`


## Running a benchmark

After compiling a benchmark

Run `make run_hbm` will run the manual transformed benchmark.

Run `make run_hbc` will run the compiled benchmark.

The runtime (taskparts) requires the `TASKPARTS_CPU_BASE_FREQUENCY_KHZ` to be set, right now this value is hard-coded and set to be `2800000`.

One can supply other runtime options from the command line to run the benchmark. For example, to warmup the runtime 3 seconds, and run the heartbeat compiler generated binary 10 times using the heartbeat rate of 100 microseconds.

Run `TASKPARTS_BENCHMARK_WARMUP_SECS=3 TASKPARTS_BENCHMARK_NUM_REPEAT=10 TASKPARTS_KAPPA_USEC=100 make run_hbc`.


## Structure of a benchmark folder

A benchmark folder is structued as follows

- `main.cpp`: entry point for the program.

- `bench.hpp/cpp`: function declarations and definition used by the benchmark. Each benchmark has a set of different input sizes pre-defined. And has different implementations using original (serial), openmp and opencilk.

- `bench.config`: benchmark-specific options. A benchmark migh have various class of input, such as `SPMV_RANDOM` and `SPMV_ARROWHEAD` for `spmv`. A benchmark migh also have different implemenations by openmp or opencilk, such as `OPENCILK_HEURISTICS` and `OPENCILK_OUTER_FOR` for `spmv`.

- `heartbeat_manual.hpp/cpp`: heartbeat transformation implemented manually.

- `heartbeat_compiler.hpp`: outlined benchmark loops with function names start with `HEARTBEAT_`. These functions are used as the input for the heartbeat compiler.

- `heartbeat.config`: heartbeat options used for the benchmark. Here's where the default chunksize gets defined per benchmark if enabling chunking execution.

- `loop_handler.hpp/cpp`: defines the promotion logic.
