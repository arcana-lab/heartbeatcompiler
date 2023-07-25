#!/bin/bash

# environment
ROOT_DIR=`git rev-parse --show-toplevel`
source ${ROOT_DIR}/enable ;
source common.sh ;

# experiment
experiment=overhead
keyword=exectime
mkdir -p ${ROOT_DIR}/evaluation/results/${experiment} ;

########################################################
# experiment sections
baseline=true

sp_baseline=true          # overhead of outlining using software polling
sp_environment=true       # overhead of preparing environment using software polling
sp_no_chunking=true       # overhead of polling without loop chunking
sp_static_chunking=true   # overhead of polling with loop chunking
sp_aca=true               # overhead of polling with adaptive chunksize adjustment
sp_scheduling=true        # overhead of scheduling based upon adaptive chunksize adjustment

rf_baseline=true          # overhead of outlining using rollforwarding
rf_environment=true       # overhead of preparing environment using rollforwarding
rf_cost=true              # overhead of rollforwarding
rf_scheduling=true        # overhead of shceduling using rollforwarding

rf_kmod_baseline=true     # overhead of outlining using rollforwarding via kernel module
rf_kmod_environment=true  # overhead of preparing environment using rollforwarding via kernel module
rf_kmod_cost=true         # overhead of rollforwarding via kernel module
rf_kmod_scheduling=true   # overhead of scheduling using rollforwarding via kernel module

# benchmark targetted
benchmarks=(plus_reduce_array mandelbrot spmv floyd_warshall srad)
########################################################

function run_and_collect {
  local technique=${1}
  local results_path=${2}
  mkdir -p ${results_path} ;
  local output=${results_path}/output.txt

  for i in `seq 1 ${baseline_num_runs}` ; do
    WORKERS=1 taskset -c 0 make run_${technique} >> ${output} ;
  done

  collect ${results_path} ${output} ;
}

# main script
pushd . ;

# preparation
cd ${ROOT_DIR}/benchmarks ;
make link &> /dev/null ;

# run experiment per benchmark
for benchmark in ${benchmarks[@]} ; do
  if [ ${benchmark} == spmv ] ; then
    input_classes=(ARROWHEAD POWERLAW RANDOM)
  else
    input_classes=(PLACE_HODLER_SO_OTHER_BENCHMARKS_CAN_RUN)
  fi

  for input_class in ${input_classes[@]} ; do
    if [ ${benchmark} == spmv ] ; then
      results=${ROOT_DIR}/evaluation/results/${experiment}/${benchmark}_`echo -e ${input_class} | tr '[:upper:]' '[:lower:]'`
    else
      results=${ROOT_DIR}/evaluation/results/${experiment}/${benchmark}
    fi
    mkdir -p ${results} ;

    cd ${benchmark} ;
    clean ;

    # baseline
    if [ ${baseline} = true ] ; then
      clean ; make baseline INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} &> /dev/null ;
      run_and_collect baseline ${results}/baseline ;
    fi

    # sp_baseline
    if [ ${sp_baseline} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=false &> /dev/null ;
      run_and_collect hbc ${results}/sp_baseline ;
    fi

    # sp_environment
    if [ ${sp_environment} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=true ENABLE_HEARTBEAT=false CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
      run_and_collect hbc ${results}/sp_environment ;
    fi

    # sp_no_chunking
    if [ ${sp_no_chunking} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=true ENABLE_HEARTBEAT=true ENABLE_PROMOTION=false CHUNK_LOOP_ITERATIONS=false OVERHEAD_ANALYSIS=true &> /dev/null ;
      run_and_collect hbc ${results}/sp_no_chunking ;
    fi

    # sp_static_chunking
    if [ ${sp_static_chunking} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=true ENABLE_HEARTBEAT=true ENABLE_PROMOTION=false CHUNK_LOOP_ITERATIONS=true OVERHEAD_ANALYSIS=true &> /dev/null ;
      run_and_collect hbc ${results}/sp_static_chunking ;
    fi

    # sp_aca
    if [ ${sp_aca} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=true ENABLE_HEARTBEAT=true ENABLE_PROMOTION=false CHUNK_LOOP_ITERATIONS=true OVERHEAD_ANALYSIS=true ACC=true CHUNKSIZE=1 &> /dev/null ;
      run_and_collect hbc ${results}/sp_aca ;
    fi

    # sp_scheduling
    if [ ${sp_scheduling} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=true ENABLE_HEARTBEAT=true ENABLE_PROMOTION=true CHUNK_LOOP_ITERATIONS=true ACC=true CHUNKSIZE=1 &> /dev/null ;
      run_and_collect hbc ${results}/sp_scheduling ;
    fi

    # rf_baseline
    if [ ${rf_baseline} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=false ENABLE_ROLLFORWARD=true &> /dev/null ;
      run_and_collect hbc ${results}/rf_baseline ;
    fi

    # rf_environment
    if [ ${rf_environment} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=true ENABLE_HEARTBEAT=false ENABLE_ROLLFORWARD=true CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
      run_and_collect hbc ${results}/rf_environment ;
    fi

    # rf_cost
    if [ ${rf_cost} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=true ENABLE_HEARTBEAT=true ENABLE_PROMOTION=false ENABLE_ROLLFORWARD=true CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
      run_and_collect hbc ${results}/rf_cost ;
    fi

    # rf_scheduling
    if [ ${rf_scheduling} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=true ENABLE_HEARTBEAT=true ENABLE_PROMOTION=true ENABLE_ROLLFORWARD=true CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
      run_and_collect hbc ${results}/rf_scheduling ;
    fi

    # rf_kmod_baseline
    if [ ${rf_kmod_baseline} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=false ENABLE_ROLLFORWARD=true USE_HB_KMOD=true &> /dev/null ;
      run_and_collect hbc ${results}/rf_kmod_baseline ;
    fi

    # rf_kmod_environment
    if [ ${rf_kmod_environment} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=true ENABLE_HEARTBEAT=false ENABLE_ROLLFORWARD=true USE_HB_KMOD=true CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
      run_and_collect hbc ${results}/rf_kmod_environment ;
    fi

    # rf_kmod_cost
    if [ ${rf_kmod_cost} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=true ENABLE_HEARTBEAT=true ENABLE_PROMOTION=false ENABLE_ROLLFORWARD=true USE_HB_KMOD=true CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
      run_and_collect hbc ${results}/rf_kmod_cost ;
    fi

    # rf_kmod_scheduling
    if [ ${rf_kmod_scheduling} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=true ENABLE_HEARTBEAT=true ENABLE_PROMOTION=true ENABLE_ROLLFORWARD=true USE_HB_KMOD=true CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
      run_and_collect hbc ${results}/rf_kmod_scheduling ;
    fi

    clean ;
    cd ../ ;
  done
done

popd ;
