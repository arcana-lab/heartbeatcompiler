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
hbc=true                    # overhead of context setup
hbc_sp_no_chunking=true     # overhead of software polling without chunking
hbc_sp_static_chunking=true # overhead of software polling using static chunksize
hbc_rf=true                 # overhead of rollforward with interrupt ping thread
hbc_rf_kmod=true            # overhead of rollforward with kernel module
hbc_sp_scheduling=true      # overhead of scheduling overhead, using default static chunking

# benchmark targetted
benchmarks=(mandelbrot spmv floyd_warshall plus_reduce_array srad)
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

    # hbc
    if [ ${hbc} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_HEARTBEAT=false &> /dev/null ;
      run_and_collect hbc ${results}/hbc ;
    fi

    # hbc_sp_no_chunking
    if [ ${hbc_sp_no_chunking} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_PROMOTION=false CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
      run_and_collect hbc ${results}/hbc_sp_no_chunking ;
    fi

    # hbc_sp_static_chunking
    if [ ${hbc_sp_static_chunking} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_PROMOTION=false &> /dev/null ;
      run_and_collect hbc ${results}/hbc_sp_static_chunking ;
    fi

    # hbc_rf
    if [ ${hbc_rf} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_PROMOTION=false ENABLE_ROLLFORWARD=true &> /dev/null ;
      run_and_collect hbc ${results}/hbc_rf ;
    fi

    # hbc_rf_kmod
    if [ ${hbc_rf_kmod} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_PROMOTION=false ENABLE_ROLLFORWARD=true USE_HB_KMOD=true &> /dev/null ;
      run_and_collect hbc ${results}/hbc_rf_kmod ;
    fi

    # hbc_sp_scheduling
    if [ ${hbc_sp_scheduling} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} &> /dev/null ;
      run_and_collect hbc ${results}/hbc_sp_scheduling ;
    fi

    clean ;
    cd ../ ;
  done
done

popd ;
