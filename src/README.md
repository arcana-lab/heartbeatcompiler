# Heartbeat Compiler

## Table of Content
- [Dependency](#dependency)
- [Building](#building)
- [Compile a benchmark](#compiling-a-benchmark)
- [Compiling options](#compiling-options)
  - [General options](#general-options)
  - [Heartbeat-specific options](#heartbeat-specific-options)
  - [Chunk size options](#chunk-size-options)
  - [Benchmark-specific options](#benchmark-specific-options)
  - [Compiling example](#compiling-example)
- [Run a benchmark](#running-a-benchmark)
  - [Runtime options](#runtime-options)
  - [Running example](#running-example)
- [Structure of a benchmark folder](#structure-of-a-benchmark-folder)

## Dependency

The following dependence is required when building heartbeat compiler
* [LLVM 15.0.0](https://releases.llvm.org/download.html)


## Building

Clone the repo and from the root directory, run `make ; source noelle/enable`


## Compile a benchmark

All benchmarks is under the folder `benchmarks`.

Run `make hbm` will compile the manually transformed benchmark using `clang`.

Run `make hbc` will compile the benchmark using the heartbeat compiler.

Run `make { baseline, opencilk, openmp }` will compile the corresponding implementation of the benchmark.


## Compiling options

Several options can be applied during the compilation stage to control how the final binary behaves

### General options
- `INPUT_SIZE`: select the input size used by the benchmark, available options are `benchmarking`, `tpal`, `testing` and `user`. The default is `testing`.

- `TEST_CORRECTNESS`: this enables running a serial version of the benchmark in the end and compare the output result against any implemenation and report the output is either CORRECT or INCORRECT. The default is `false`.

### OpenMP-specific options

- `OMP_SCHEDULE`: decide which openmp schedule to use, available options are `STATIC`, `DYNAMIC` and `GUIDED`. The default is `STATIC`.

- `OMP_NESTED_PARALLELISM`: decide whether to enable nested parallelism on all loop levels. The default is `false` and OpenMP only parallelizes the outermost loop.

- `OMP_CHUNKSIZE`: decide the user-specified chunk size used for the openmp scheduler. The program uses the default chunk size per openmp scheduler if this value is not specified.

### Heartbeat-specific options
- `ENABLE_PROMOTION`: decide whether to enable parallelism promotion. loop_handler will return 0 immediately if this option is disabled. The default is `true`.

- `ENABLE_ROLLFORWARD`: this controls wether or not to compile the benchmark using the rollforwarding compilation. If disabled, the program will use software polling to determine whether a heartbeat event arrives or not. The default is `false`.

- `CHUNK_LOOP_ITERATIONS`: whether to execute the loop in chunk before calling loop_handler. The default is `true`.

- `ADAPTIVE_CHUNKSIZE_CONTROL`: whether to do adaptive chunk size control to minimize the software polling overhead. The default is `false`.

- `SMALLE_GRANULARITY`: the minimal remaining iteration size for a loop to split its work and promote parallelism. The default is `2`.

### Chunk size options
The `CHUNKSIZE` option is used as a static value for all leaf loops targeted by HBC. If not supplied, the compilation uses the default chunk size provided in `heartbeat.config` per benchmark folder. Chunk size should not be less than `1` to avoid an infinite loop. If using `SOFTWARE_POLLING` as the mechanism to detect heartbeat and `ADAPTIVE_CHUNKSIZE_CONTROL` is turned on, the chunk size will change during program runtime and the `CHUNKSIZE` option specifies the initial chunk size.

### Benchmark-specific options
The following benchmarks have specific compilation options
- `spmv`:
  - `INPUT_CLASS` could be one of the options in `SPMV_{RANDOM, POWERLAW, ARROWHEAD, DENSE, DIAGONAL, NORMAL}`. The default is `SPMV_RANDOM`.
  - `OPENCILK_IMPL`: what opencilk implementation used for the spmv, could be one of the options in `OPENCILK_{REDUCER, MANUAL_GRANULARITY, OUTER_FOR, HEURISTICS}`. The defualt is `OPENCILK_HEURISTICS`.

### Compiling Example

1. compile the `plus_reduce_array` using TPAL's input size and verify the correctness of the implementation with no chunking execution: `cd plus_reduce_array ; INPUT_SIZE=tpal TEST_CORRECTNESS=true CHUNK_LOOP_ITERATIONS=false make hbc`

2. compile the `spmv` using TPAL's input size and the arrowhead input class, plus wants to experiment a different chunk size for the innermost loop using the software polling mechanism: `cd spmv ; INPUT_SIZE=tpal INPUT_CLASS=SPMV_ARROWHEAD CHUNKSIZE=2048 make hbc`


## Run a benchmark

After compiling a benchmark

Run `make run_hbm` will run the manual transformed benchmark.

Run `make run_hbc` will run the compiled benchmark.

Run `make run_{ baseline, opencilk, openmp }` will run the corresponding implementation of the benchmark.

### Runtime options

- `WORKERS`: the number of threads to run the parallel implementation of the benchmark. If not supplied, runtime is suppose to explore the maximal concurrency.

- `WARMUP_SECS`: seconds to run the binary to warmup the system, default is `0.0`.

- `NUM_REPEAT`: number of runs of the benchmark, default is `1`.

- `VERBOSE`: whether to print extra message when running the benchmark, default is `false`.

- `CPU_FREQUENCY_KHZ`: cpu frequency in khz, needed by the taskparts runtime, default is `3000000`.

- `KAPPA_USECS`: the heartbeat rate is microseconds, default is `100`.

### Running example

1. run `plus_reduce_array`'s openmp implementation using 64 cores and repeat 5 times: `WORKERS=64 NUM_REPEAT=5 make run_openmp`

2. run `spmv`'s heartbeat implementation using 32 cores and set the heartbeat rate to be 300 microseconds: `WORKERS=32 KAPPA_USECS=300 make run_hbc`


## Structure of a benchmark folder

A benchmark folder is structued as follows

- `main.cpp`: entry point for the program.

- `bench.hpp/cpp`: function declarations and definition used by the benchmark. Each benchmark has a set of different input sizes pre-defined. And has different implementations using original (serial), openmp and opencilk.

- `bench.config`: benchmark-specific options. A benchmark migh have various class of input, such as `SPMV_RANDOM` and `SPMV_ARROWHEAD` for `spmv`. A benchmark migh also have different implemenations by openmp or opencilk, such as `OPENCILK_HEURISTICS` and `OPENCILK_OUTER_FOR` for `spmv`. Specific macros/flags/opts/libs used by a benchmark should go here as well.

- `heartbeat_manual.hpp/cpp`: heartbeat transformation implemented manually.

- `heartbeat_compiler.hpp`: outlined benchmark loops with function names start with `HEARTBEAT_`. These functions are used as the input for the heartbeat compiler.

- `heartbeat.config`: heartbeat options used for the benchmark. Here's where the default chunk size gets defined per benchmark if enabling chunking execution.

- `loop_handler.hpp/cpp`: defines the promotion logic.
