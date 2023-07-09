ROOT_DIR = $(shell git rev-parse --show-toplevel)
BENCHMARK = $(shell basename $(CURDIR))

CC = clang
CXX = clang++

# ============================================
# TASKPARTS definitions
# ============================================
TASKPARTS_PATH = $(ROOT_DIR)/runtime
TASKPARTS_INCLUDE = $(TASKPARTS_PATH)/include
TASKPARTS_PLATFORM ?= -DTASKPARTS_POSIX -DTASKPARTS_X64

# ============================================
# Global MACROS used by all implementations
# ============================================
# size of input, either benchmarking, tpal or testing
# options: benchmarking, tpal, testing
INPUT_SIZE ?= testing
ifeq ($(INPUT_SIZE), benchmarking)
	MACROS += -DINPUT_BENCHMARKING
else ifeq ($(INPUT_SIZE), tpal)
	MACROS += -DINPUT_TPAL
else ifeq ($(INPUT_SIZE), testing)
	MACROS += -DINPUT_TESTING
else ifeq ($(INPUT_SIZE), user)
	MACROS += -DINPUT_USER
endif

# whether to test the correctness of any implementation
TEST_CORRECTNESS ?= false
ifeq ($(TEST_CORRECTNESS), true)
	MACROS += -DTEST_CORRECTNESS
endif

# whether to enable debug mode
DEBUG ?= false
ifeq ($(DEBUG), true)
	MACROS += -DDEBUG
endif

# whether to print compilation configurations
STATS ?= false
ifeq ($(STATS), true)
	MACROS += -DSTATS
endif

# include benchmark specific settings
-include bench.config

MACROS += $(TASKPARTS_PLATFORM)

# ============================================
# Global FLAGS used by all implementations
# ============================================
FLAGS += \
	--std=c++17 \
	-I$(TASKPARTS_INCLUDE)

# ============================================
# Global OPTS used by all implementations
# ============================================
ifeq ($(DEBUG), false)
	OPTS += -O3 -march=native
else
	OPTS += -O0 -g
endif

OPTS += \
	-fno-stack-protector \
	-fno-asynchronous-unwind-tables \
	-fno-verbose-asm -fno-rtti -fno-pic -fpie \
	-fno-vectorize -fno-slp-vectorize

# ============================================
# Global LIBS used by all implementations
# ============================================
LIBS = -lm -lstdc++ -pthread -ldl
ifeq (, $(shell which jemalloc-config))
	CUSTOM_MALLOC =
else
	JEMALLOCLD = $(shell jemalloc-config --libdir)
	CUSTOM_MALLOC = -L$(JEMALLOCLD) -Wl,-rpath,$(JEMALLOCLD) -ljemalloc
	LIBS += $(CUSTOM_MALLOC)
endif

# =============================================
# Running options
# =============================================
WORKERS ?= 0
WARMUP_SECS ?= 0.0
NUM_REPEAT ?= 1
VERBOSE ?= 0
CPU_FREQUENCY_KHZ ?= 3000000
KAPPA_USECS ?=100

# =============================================
# processing
# =============================================
preprocess: utility.hpp
	sed -i '/std::string benchmark_name/c\std::string benchmark_name = \"$(BENCHMARK)\";' $(ROOT_DIR)/benchmarks/commons/utility.hpp ;

postprocess: utility.hpp
	sed -i '/std::string benchmark_name/c\std::string benchmark_name = \"\";' $(ROOT_DIR)/benchmarks/commons/utility.hpp ;

# ============================================
# Baseline
# ============================================
MACROS_BASELINE += -DUSE_BASELINE $(MACROS)
FLAGS_BASELINE += $(FLAGS)
OPTS_BASELINE += $(OPTS)
LIBS_BASELINE += $(LIBS)

baseline: main.cpp bench.cpp
	make preprocess
	$(CXX) $(MACROS_BASELINE) $(FLAGS_BASELINE) $(OPTS_BASELINE) $^ -o $@ $(LIBS_BASELINE)
	make postprocess

run_baseline: baseline
	UTILITY_BENCHMARK_WARMUP_SECS=$(WARMUP_SECS) \
	UTILITY_BENCHMARK_NUM_REPEAT=$(NUM_REPEAT) \
	UTILITY_BENCHMARK_VERBOSE=$(VERBOSE) \
	./$< $(ARGS)

# ============================================
# OpenCilk
# ============================================
OPENCILK_COMPILER ?= /nfs-scratch/yso0488/opencilk/build/bin/bin/opencilk++

MACROS_OPENCILK += -DUSE_OPENCILK $(MACROS)
FLAGS_OPENCILK += -fopencilk $(FLAGS)
OPTS_OPENCILK += $(OPTS)
LIBS_OPENCILK += $(LIBS)

opencilk: main.cpp bench.cpp
	make preprocess
	$(OPENCILK_COMPILER) $(MACROS_OPENCILK) $(FLAGS_OPENCILK) $(OPTS_OPENCILK) $^ -o $@ $(LIBS_OPENCILK)
	make postprocess

run_opencilk: opencilk
ifneq ($(WORKERS), 0)
	CILK_NWORKERS=$(WORKERS) \
	UTILITY_BENCHMARK_WARMUP_SECS=$(WARMUP_SECS) \
	UTILITY_BENCHMARK_NUM_REPEAT=$(NUM_REPEAT) \
	UTILITY_BENCHMARK_VERBOSE=$(VERBOSE) \
	./$< $(ARGS)
else
	UTILITY_BENCHMARK_WARMUP_SECS=$(WARMUP_SECS) \
	UTILITY_BENCHMARK_NUM_REPEAT=$(NUM_REPEAT) \
	UTILITY_BENCHMARK_VERBOSE=$(VERBOSE) \
	./$< $(ARGS)
endif

# ============================================
# CilkPlus
# ============================================
MACROS_CILKPLUS += -DUSE_CILKPLUS $(MACROS)
FLAGS_CILKPLUS += -fcilkplus $(FLAGS)
OPTS_CILKPLUS += \
	-fno-stack-protector \
	-fno-asynchronous-unwind-tables \
	-fno-verbose-asm -fno-rtti -fno-pic -fpie \
	-fno-tree-vectorize
LIBS_CILKPLUS += $(LIBS)

cilkplus: main.cpp bench.cpp
	gcc $(MACROS_CILKPLUS) $(FLAGS_CILKPLUS) $(OPTS_CILKPLUS) $^ -o $@ $(LIBS_CILKPLUS)

run_cilkplus: cilkplus
ifneq ($(WORKERS), 0)
	CILK_NWORKERS=$(WORKERS) \
	UTILITY_BENCHMARK_WARMUP_SECS=$(WARMUP_SECS) \
	UTILITY_BENCHMARK_NUM_REPEAT=$(NUM_REPEAT) \
	UTILITY_BENCHMARK_VERBOSE=$(VERBOSE) \
	./$< $(ARGS)
else
	UTILITY_BENCHMARK_WARMUP_SECS=$(WARMUP_SECS) \
	UTILITY_BENCHMARK_NUM_REPEAT=$(NUM_REPEAT) \
	UTILITY_BENCHMARK_VERBOSE=$(VERBOSE) \
	./$< $(ARGS)
endif

# ============================================
# OpenMP
# ============================================
MACROS_OPENMP += -DUSE_OPENMP $(MACROS)
FLAGS_OPENMP += -fopenmp $(FLAGS)
OPTS_OPENMP += $(OPTS)
LIBS_OPENMP += -fopenmp $(LIBS)

OMP_SCHEDULE ?= STATIC
ifeq ($(OMP_SCHEDULE), STATIC)
	FLAGS_OPENMP += -DOMP_SCHEDULE_STATIC
else ifeq ($(OMP_SCHEDULE), DYNAMIC)
	FLAGS_OPENMP += -DOMP_SCHEDULE_DYNAMIC
else ifeq ($(OMP_SCHEDULE), GUIDED)
	FLAGS_OPENMP += -DOMP_SCHEDULE_GUIDED
endif

OMP_NESTED_SCHEDULING ?= false
ifeq ($(OMP_NESTED_SCHEDULING), true)
	FLAGS_OPENMP += -DOMP_NESTED_SCHEDULING
endif

openmp: main.cpp bench.cpp
	make preprocess
	$(CXX) $(MACROS_OPENMP) $(FLAGS_OPENMP) $(OPTS_OPENMP) $^ -o $@ $(LIBS_OPENMP)
	make postprocess

run_openmp: openmp
ifneq ($(WORKERS), 0)
	OMP_NUM_THREADS=$(WORKERS) \
	UTILITY_BENCHMARK_WARMUP_SECS=$(WARMUP_SECS) \
	UTILITY_BENCHMARK_NUM_REPEAT=$(NUM_REPEAT) \
	UTILITY_BENCHMARK_VERBOSE=$(VERBOSE) \
	./$< $(ARGS)
else
	UTILITY_BENCHMARK_WARMUP_SECS=$(WARMUP_SECS) \
	UTILITY_BENCHMARK_NUM_REPEAT=$(NUM_REPEAT) \
	UTILITY_BENCHMARK_VERBOSE=$(VERBOSE) \
	./$< $(ARGS)
endif

# ============================================
# Heartbeat
# ============================================
HEARTBEAT_LINUX_PATH = $(ROOT_DIR)/heartbeat-linux
HEARTBEAT_LINUX_INCLUDE =$(HEARTBEAT_LINUX_PATH)/user
ROLLFORWARD_PATH = $(ROOT_DIR)/rollforward/src
ROLLFORWARD_INCLUDE = $(ROLLFORWARD_PATH)/x64
ROLLFORWARD_COMPILER_PATH = $(TASKPARTS_PATH)/benchmark/tpal
ROLLFORWARD_COMPILER = $(ROLLFORWARD_COMPILER_PATH)/rollforward++

MACROS_HEARTBEAT += $(MACROS)

# whether to enable heartbeat transformation
# thie macro is used to measure the overhead of
# preparing loop environment and parallel context
# this macro applies to heartbeat_manual only
# and therefore can only be measured using heartbeat_manual
ENABLE_HEARTBEAT ?= true
ifeq ($(ENABLE_HEARTBEAT), true)
	MACROS_HEARTBEAT += -DENABLE_HEARTBEAT
else
	MACROS_HEARTBEAT += -DDISABLE_HEARTBEAT
	PASS_ARGUMENTS_HEARTBEAT_COMPILER += -disable_heartbeat
endif

# whether to enable promotion
ENABLE_PROMOTION ?= true
PROMO_STATS ?= false	# collect the level of where parallelism is generated
ifeq ($(ENABLE_PROMOTION), true)
	MACROS_HEARTBEAT += -DENABLE_PROMOTION
	ifeq ($(PROMO_STATS), true)
		MACROS_HEARTBEAT += -DPROMO_STATS
	endif
else
	MACROS_HEARTBEAT += -DDISABLE_PROMOTION
endif

# whether to enable the rollforward compilation
ENABLE_ROLLFORWARD ?= false
USE_HB_KMOD ?= false	# whether to use the heartbeat linux module
ifeq ($(ENABLE_ROLLFORWARD), true)
	MACROS_HEARTBEAT += -DENABLE_ROLLFORWARD
	PASS_ARGUMENTS_HEARTBEAT_COMPILER += -enable_rollforward
else
	MACROS_HEARTBEAT += -DENABLE_SOFTWARE_POLLING
endif

# whether to chunk loop iterations before calling loop_handler
CHUNK_LOOP_ITERATIONS ?= true
ACC ?= false						# enable adaptive chunksize control
ACC_DEBUG ?= false			# print how chunksize changes over polling count
ACC_EVAL ?= false				# evaluate the accuracy of acc
ACC_SPMV_STATS ?= false	# plot how chunksize changes varying the number of nonzeros
ifeq ($(CHUNK_LOOP_ITERATIONS), true)
	MACROS_HEARTBEAT += -DCHUNK_LOOP_ITERATIONS
	PASS_ARGUMENTS_HEARTBEAT_COMPILER += -chunk_loop_iterations
	ifeq ($(ENABLE_ROLLFORWARD), false)	# ACC can only be enabled when not using rollforwarding
		ifeq ($(ACC), true)
			MACROS_HEARTBEAT += -DADAPTIVE_CHUNKSIZE_CONTROL
			PASS_ARGUMENTS_HEARTBEAT_COMPILER += -adaptive_chunksize_control
			ifeq ($(ACC_DEBUG), true)
				MACROS_HEARTBEAT += -DACC_DEBUG
			endif
			ifeq ($(ACC_EVAL), true)
				MACROS_HEARTBEAT += -DACC_EVAL
			endif
			ifeq ($(ACC_SPMV_STATS), true)
				MACROS_HEARTBEAT += -DACC_SPMV_STATS
			endif
		endif
	endif
endif

# whether to print polling stats
POLLS_STATS ?= false
ifeq ($(ENABLE_ROLLFORWARD), false)
	ifeq ($(POLLS_STATS), true)
		MACROS += -DPOLLS_STATS
	endif
endif

# include benchmark heartbeat specific settings
-include heartbeat.config

# macros need to enable taskparts runtime
MACROS_HEARTBEAT += \
	-DTASKPARTS_TPALRTS \
	-DTASKPARTS_STATS \
	-DTASKPARTS_USE_CHASELEV_DEQUE
ifeq ($(ENABLE_ROLLFORWARD), true)
	ifeq ($(USE_HB_KMOD), true)
		MACROS_HEARTBEAT += \
			-DUSE_HB_KMOD \
			-DTASKPARTS_TPALRTS_HBTIMER_KMOD # required by the linux kernel module
	endif
else
	MACROS_HEARTBEAT += -DTASKPARTS_TPALRTS_HARDWARE_ALARM_POLLING # to enable software polling
endif

FLAGS_HEARTBEAT += $(FLAGS)
ifeq ($(ENABLE_ROLLFORWARD), true)
	FLAGS_HEARTBEAT += -I$(ROLLFORWARD_INCLUDE)
	ifeq ($(USE_HB_KMOD), true)
		FLAGS_HEARTBEAT += -I$(HEARTBEAT_LINUX_INCLUDE)
	endif
endif

OPTS_HEARTBEAT += $(OPTS)

ifeq ($(ENABLE_ROLLFORWARD), true)
	ifeq ($(USE_HB_KMOD), true)
		LIBS_HEARTBEAT += -L$(HEARTBEAT_LINUX_PATH) -Wl,-rpath,$(HEARTBEAT_LINUX_PATH) -lhb
	endif
endif
LIBS_HEARTBEAT += $(LIBS)

-include Makefile_heartbeat_manual
-include Makefile_heartbeat_compiler

# ============================================
# Evaluation
# ============================================
condor:
	cd condor ; condor_submit job.con ;

condor_clean:
	cd condor ; rm -rf condor.* ;

# ============================================
# Other
# ============================================
clean:
	rm -rf baseline opencilk cilkplus openmp ;
	rm -rf heartbeat_manual heartbeat_manual_rf ;
	rm -rf heartbeat_compiler heartbeat_compiler_rf ;
	rm -rf *.s *.o ;
	rm -rf *.bc *.ll ;
	rm -rf loopIDtoSrc.txt ;

.PHONY: condor clean
