ROOT_DIR = $(shell git rev-parse --show-toplevel)
HBC_PATH = $(ROOT_DIR)/heartbeatcompiler
BENCHMARK = $(shell basename $(CURDIR))

CC = clang
CXX = clang++

# ============================================
# TACO definitions
# ============================================
TACO_PATH = $(ROOT_DIR)/taco
TACO_INCLUDE = $(TACO_PATH)/include
TACO_MACROS = -DNDEBUG -DTACO

# ============================================
# TASKPARTS definitions
# ============================================
TASKPARTS_PATH = $(HBC_PATH)/runtime
TASKPARTS_INCLUDE = $(TASKPARTS_PATH)/include
TASKPARTS_PLATFORM ?= -DTASKPARTS_POSIX -DTASKPARTS_X64

# ============================================
# Global MACROS used by all implementations
# ============================================
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

# include benchmark specific settings
-include bench.config

MACROS += $(TACO_MACROS) \
	$(TASKPARTS_PLATFORM) \
	-DTASKPARTS_TPALRTS \
	-DTASKPARTS_STATS \
	-DTASKPARTS_USE_CHASELEV_DEQUE

# ============================================
# Global FLAGS used by all implementations
# ============================================
FRONTEND_OPTIONS += \
	-O1 -emit-llvm \
	-Xclang -disable-llvm-passes \
	-Xclang -ffp-contract=off

FLAGS += \
	-I$(TACO_INCLUDE) \
	-I$(TASKPARTS_INCLUDE) \
	--std=c++17 \
	-fno-stack-protector \
	-fno-asynchronous-unwind-tables \
	-fno-verbose-asm -frtti -fno-pic -fpie \
	-fno-vectorize -fno-slp-vectorize \
	$(FRONTEND_OPTIONS)

# ============================================
# Global OPTS used by all implementations
# ============================================
ifeq ($(DEBUG), false)
	OPTS += \
		-O3 -march=native \
		-fno-vectorize -fno-slp-vectorize
else
	FLAGS += -g
	OPTS += -O0
endif

# ============================================
# Global LIBS used by all implementations
# ============================================
LIBS += -L$(TACO_PATH)/build/lib -ltaco -lm -lstdc++ -pthread -ldl
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

# ============================================
# Baseline
# ============================================
MACROS_BASELINE += -DUSE_BASELINE $(MACROS) -DTASKPARTS_TPALRTS_MINIMAL
FLAGS_BASELINE += $(FLAGS)
OPTS_BASELINE += $(OPTS)
LIBS_BASELINE += $(LIBS)

%_baseline.bc: %.cpp
	$(CXX) $(MACROS_BASELINE) $(FLAGS_BASELINE) -c $< -o $@

baseline_normalized.bc: main_baseline.bc
	noelle-norm $< -o $@

baseline: baseline_normalized.bc
	$(CXX) $(OPTS_BASELINE) $^ -o $@ $(LIBS_BASELINE)

run_baseline: baseline
	TASKPARTS_NUM_WORKERS=1 \
	TASKPARTS_BENCHMARK_WARMUP_SECS=$(WARMUP_SECS) \
	TASKPARTS_BENCHMARK_NUM_REPEAT=$(NUM_REPEAT) \
	TASKPARTS_BENCHMARK_VERBOSE=$(VERBOSE) \
	TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(CPU_FREQUENCY_KHZ) \
	TASKPARTS_KAPPA_USEC=$(KAPPA_USECS) \
	./$< $(ARGS)

# ============================================
# OpenMP
# ============================================
MACROS_OPENMP += -DUSE_OPENMP $(MACROS)
FLAGS_OPENMP += -fopenmp $(FLAGS)
OPTS_OPENMP += $(OPTS)
LIBS_OPENMP += -fopenmp $(LIBS)

# openmp schedulers to use
# options: STATIC, DYNAMIC, GUIDED
OMP_SCHEDULE ?= STATIC
ifeq ($(OMP_SCHEDULE), STATIC)
	MACROS_OPENMP += -DOMP_SCHEDULE_STATIC
else ifeq ($(OMP_SCHEDULE), DYNAMIC)
	MACROS_OPENMP += -DOMP_SCHEDULE_DYNAMIC
else ifeq ($(OMP_SCHEDULE), GUIDED)
	MACROS_OPENMP += -DOMP_SCHEDULE_GUIDED
endif

# whether to enable nested parallelism for openmp
OMP_NESTED_PARALLELISM ?= false
ifeq ($(OMP_NESTED_PARALLELISM), true)
	MACROS_OPENMP += -DOMP_NESTED_PARALLELISM
endif

# chunk size used for openmp's different schedulers
# if unset, use the default value for different schedulers
OMP_CHUNKSIZE ?= 0
ifneq ($(OMP_CHUNKSIZE), 0)
	MACROS_OPENMP += -DOMP_CHUNKSIZE=$(OMP_CHUNKSIZE)
endif

%_openmp.bc: %.cpp
	$(CXX) $(MACROS_OPENMP) $(FLAGS_OPENMP) -c $< -o $@

openmp_normalized.bc: main_openmp.bc
	noelle-norm $< -o $@

openmp: openmp_normalized.bc
	$(CXX) $(OPTS_OPENMP) $^ -o $@ $(LIBS_OPENMP)

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
ROLLFORWARD_PATH = $(HBC_PATH)/rollforward/src
ROLLFORWARD_INCLUDE = $(ROLLFORWARD_PATH)/x64
ROLLFORWARD_COMPILER_PATH = $(TASKPARTS_PATH)/benchmark/tpal
ROLLFORWARD_COMPILER = $(ROLLFORWARD_COMPILER_PATH)/rollforward++

MACROS_HEARTBEAT += $(MACROS)

# whether to run code of heartbeat transformation
# if set to false, then it will invoke the
# original outlined loops without modifcation
# this macro can be used to evaluate the outlining
# cost of the overhead analysis
RUN_HEARTBEAT ?= true
ifeq ($(RUN_HEARTBEAT), true)
	MACROS_HEARTBEAT += -DRUN_HEARTBEAT
endif

# whether to enable heartbeat transformation
# thie macro is used to measure the overhead of
# preparing loop environment and parallel context,
# therefore, if set to false, no promotion_handler will
# get inserted.
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
TASK_STATS ?= false		# collect the number of tasks created
ifeq ($(ENABLE_PROMOTION), true)
	MACROS_HEARTBEAT += -DENABLE_PROMOTION
	ifeq ($(PROMO_STATS), true)
		MACROS_HEARTBEAT += -DPROMO_STATS
	endif
	ifeq ($(TASK_STATS), true)
		MACROS_HEARTBEAT += -DTASK_STATS
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

# whether to chunk loop iterations before calling promotion_handler
CHUNK_LOOP_ITERATIONS ?= true
ACC ?= false						# enable adaptive chunksize control
ACC_MINIMAL ?= true			# enable acc conservatively to stay faithful to heartbeat scheduling
ACC_DEBUG ?= false			# print how chunksize changes over polling count
ACC_EVAL ?= false				# evaluate the accuracy of acc
ACC_SPMV_STATS ?= false	# plot how chunksize changes varying the number of nonzeros
SPMV_DETECTION_RATE_ANALYSIS ?= false # parallel outer loop only to simulate heterogeneous latency
ifeq ($(CHUNK_LOOP_ITERATIONS), true)
	MACROS_HEARTBEAT += -DCHUNK_LOOP_ITERATIONS
	PASS_ARGUMENTS_HEARTBEAT_COMPILER += -chunk_loop_iterations
	ifeq ($(ENABLE_ROLLFORWARD), false)	# ACC can only be enabled when not using rollforwarding
		ifeq ($(ACC), true)
			MACROS_HEARTBEAT += -DADAPTIVE_CHUNKSIZE_CONTROL
			PASS_ARGUMENTS_HEARTBEAT_COMPILER += -adaptive_chunksize_control
			ifeq ($(ACC_MINIMAL), false)
				MACROS_HEARTBEAT += -DACC_MAXIMAL
			else
				MACROS_HEARTBEAT += -DACC_MINIMAL
			endif
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
		ifeq ($(SPMV_DETECTION_RATE_ANALYSIS), true)
			MACROS_HEARTBEAT += -DSPMV_DETECTION_RATE_ANALYSIS
		endif
	endif
endif

POLLS_STATS ?= false			# whether to print polling stats between beats
BEATS_STATS ?= false			# print the info related to polling count and heartbeat detection rate
ifeq ($(ENABLE_ROLLFORWARD), false)
	ifeq ($(POLLS_STATS), true)
		MACROS += -DPOLLS_STATS
	endif
	ifeq ($(BEATS_STATS), true)
		MACROS += -DBEATS_STATS
	endif
endif

# this enables AC logic without doing the actual promotion
# considering changing this macro to be more defined somewhere
OVERHEAD_ANALYSIS ?= false
ifeq ($(OVERHEAD_ANALYSIS), true)
	MACROS += -DOVERHEAD_ANALYSIS
endif
PROMOTION_INSERTION_OVERHEAD_ANALYSIS ?= false
ifeq ($(PROMOTION_INSERTION_OVERHEAD_ANALYSIS), true)
	MACROS += -DPROMOTION_INSERTION_OVERHEAD_ANALYSIS
endif
CHUNKSIZE_TRANSFERRING_OVERHEAD_ANALYSIS ?= false
ifeq ($(CHUNKSIZE_TRANSFERRING_OVERHEAD_ANALYSIS), true)
	MACROS += -DCHUNKSIZE_TRANSFERRING_OVERHEAD_ANALYSIS
endif

# include benchmark heartbeat specific settings
-include heartbeat.config

# macros need to enable taskparts runtime
ifeq ($(ENABLE_ROLLFORWARD), true)
	ifeq ($(USE_HB_KMOD), true)
		MACROS_HEARTBEAT += \
			-DUSE_HB_KMOD \
			-DTASKPARTS_TPALRTS_HBTIMER_KMOD # required by the linux kernel module
	endif
else
	MACROS_HEARTBEAT += -DTASKPARTS_TPALRTS_MINIMAL
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

# ============================================
# Heartbeat Compiler
# ============================================
MACROS_HEARTBEAT_COMPILER += -DUSE_HB_COMPILER $(MACROS_HEARTBEAT)
FLAGS_HEARTBEAT_COMPILER += $(FLAGS_HEARTBEAT)
OPTS_HEARTBEAT_COMPILER += $(OPTS_HEARTBEAT)
LIBS_HEARTBEAT_COMPILER += $(LIBS_HEARTBEAT)

%_hbc.bc: %.cpp
	$(CXX) $(MACROS_HEARTBEAT_COMPILER) $(FLAGS_HEARTBEAT_COMPILER) -c $< -o $@

heartbeat_compiler_normalized.bc: main_hbc.bc
	noelle-norm $< -o $@

heartbeat_compiler.bc: heartbeat_compiler_normalized.bc
ifeq ($(ENABLE_ROLLFORWARD), true)
	make rf_test.bc
	llvm-link rf_test.bc $< -o $<
endif
	noelle-parallel-load -noelle-verbose=2 -load $(ROOT_DIR)/heartbeatcompiler/build/lib/Heartbeat.so -heartbeat $(PASS_ARGUMENTS_HEARTBEAT_COMPILER) $^ -o $@

hbc: heartbeat_compiler
hb_compiler: heartbeat_compiler
heartbeat_compiler: heartbeat_compiler.bc promotion_handler_hbc.bc
ifeq ($(ENABLE_ROLLFORWARD), true)
	make heartbeat_compiler_rf
	cp heartbeat_compiler_rf $@
else
	llvm-link $^ -o $@.bc
	$(CXX) $(OPTS_HEARTBEAT_COMPILER) $@.bc -o $@ $(LIBS_HEARTBEAT_COMPILER)
endif

run_hbc: run_heartbeat_compiler
run_hb_compiler: run_heartbeat_compiler
run_heartbeat_compiler: heartbeat_compiler
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
# Heartbeat Compiler (Rollforward)
# ============================================
rf_test.bc: rf_test.cpp
	$(CXX) -O3 -emit-llvm -Xclang -no-opaque-pointers -c $< -o $@

heartbeat_compiler.s: heartbeat_compiler.bc
	$(CXX) $(OPTS_HEARTBEAT_COMPILER) $< -S -o $@

heartbeat_compiler.rf.s: heartbeat_compiler.s
	ROLLFORWARD_PATH=$(ROLLFORWARD_PATH) $(ROLLFORWARD_COMPILER) $< $@

heartbeat_compiler.rf.o: heartbeat_compiler.rf.s
	$(CXX) -I $(ROLLFORWARD_INCLUDE) $< -c -o $@

# resolve the "undefined reference to __rf_signal" error
heartbeat_compiler.rf.ld.o: heartbeat_compiler.rf.o
	ld -r --defsym=__rf_signal=0x42 $< -o $@

rollforward_compiler.o: $(ROLLFORWARD_INCLUDE)/rollforward.c
	$(CXX) $< -c -o $@

heartbeat_compiler_rf: heartbeat_compiler.rf.ld.o rollforward_compiler.o promotion_handler_hbc.bc
	$(CXX) $(OPTS_HEARTBEAT_COMPILER) $^ -o $@ $(LIBS_HEARTBEAT_COMPILER)

# ============================================
# Heartbeat Manual
# ============================================
MACROS_HEARTBEAT_MANUAL += -DUSE_HB_MANUAL $(MACROS_HEARTBEAT)
FLAGS_HEARTBEAT_MANUAL += $(FLAGS_HEARTBEAT)
OPTS_HEARTBEAT_MANUAL += $(OPTS_HEARTBEAT)
LIBS_HEARTBEAT_MANUAL += $(LIBS_HEARTBEAT)

%_hbm.bc: %.cpp
	$(CXX) $(MACROS_HEARTBEAT_MANUAL) $(FLAGS_HEARTBEAT_MANUAL) -c $< -o $@

heartbeat_manual_normalized.bc: main_hbm.bc
	noelle-norm $< -o $@

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

heartbeat_manual_rf: heartbeat_manual.rf.ld.o rollforward_manual.o promotion_handler_hbm.bc
	$(CXX) $(OPTS_HEARTBEAT_MANUAL) $^ -o $@ $(LIBS_HEARTBEAT_MANUAL)

# ============================================
# Other
# ============================================
clean:
	rm -rf baseline openmp ;
	rm -rf heartbeat_manual heartbeat_manual_rf ;
	rm -rf heartbeat_compiler heartbeat_compiler_rf ;
	rm -rf *.s *.o ;
	rm -rf *.bc *.ll ;
	rm -rf loopIDtoSrc.txt i.txt nonzeros.txt chunksizes.txt ;

.PHONY: clean
