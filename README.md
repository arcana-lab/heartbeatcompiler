# Heartbeat Compiler (HBC)


## Table of Contents
- [Description](#description)
- [Prerequisites](#prerequisites)
- [Building HBC](#building-hbc)
- [Uninstalling HBC](#uninstalling-hbc)
- [Repository structure](#repository-structure)
- [Examples of using HBC](#examples-of-using-hbc)
- [License](#license)


## Description
HBC is the first compiler that transforms loop-based nested parallelism into binaries that benefit from heartbeat scheduling.
HBC is built upon [NOELLE](https://github.com/arcana-lab/noelle) and [LLVM](http://llvm.org).

HBC is in active development, so more optimizations will be added as we develop them.
Moreover, please be patient if you find problems, and please report them to us; we will do our best to fix them.

We release HBC's source code in the hope others will benefit from it.
You are kindly asked to acknowledge usage of the tool by citing the following paper:
```
@inproceedings{HBC,
    title={Compiling Loop-Based Nested Parallelism for Irregular Workloads},
    author={Yian Su and Mike Rainey and Nick Wanninger and Nadharm Dhiantravan and Jasper Liang and Umut A. Acar and Peter Dinda and Simone Campanoni},
    booktitle={the ACM International Conference on Architectural Support for Programming Languages and Operating Systems, 2024. ASPLOS 2024.},
    year={2024}
}
```

The only documentation available for HBC is:
- a [lightning talk video](https://youtu.be/nJLvu4tZblg) introducing HBC
- the [paper](https://yiansu.com/files/papers/HBC_ASPLOS_2024.pdf)
- the comments within the code


## Prerequisites
LLVM 14.0.6


## Building HBC
To build and install HBC: run `make` from the repository root directory.
This command will automatically download and install the correct version of [NOELLE](https://github.com/arcana-lab/noelle).


## Uninstalling HBC
Run `make clean` from the root directory to clean the repository.

Run `make uninstall` from the root directory to uninstall the HBC installation.


## Repository structure
The directory `src` includes source files of HBC.

The directory `commons` includes definitions of runtime calls, as well as utility functions used by the HBC runtime.

The directory `compilation` includes Makefiles that define the compilation pipeline of HBC.


## Examples of using HBC
The `example` directory contains a code example that HBC compiles.

To run the code example, first run `make link` from the root directory.

Then, in the `example` directory, run `make hbc ACC=true CHUNKSIZE=1`.

To run the compiled binary, run `WORKERS={NUM_THREADS} make run_hbc`.

If you have any trouble using HBC feel free to reach out to us for help (contact yiansu2018@u.northwestern.edu).


## Contributions
We welcome contributions from the community to improve this work and evolve it to cater for more users.


## License
HBC is licensed under the [MIT License](./LICENSE.md).
