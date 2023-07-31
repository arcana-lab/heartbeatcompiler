#!/bin/bash

# environment
ROOT_DIR=`git rev-parse --show-toplevel`
source ${ROOT_DIR}/enable ;
source common.sh ;

# experiment
experiment=target_polling_count
keyword=heartbeats
mkdir -p ${ROOT_DIR}/evaluation/results/${experiment};

########################################################
# experiment sections
target_polling_counts=(2 4 8 16 32)

# benchmark targetted
benchmarks=(mandelbrot spmv floyd_warshall plus_reduce_array srad)
########################################################

function run_and_collect {
  local results_path=${1}
  local target_polling_count=${2}
  mkdir -p ${results_path} ;
  local output=${results_path}/output.txt

  for i in `seq 1 3` ; do
    WORKERS=1 \
    CPU_FREQUENCY_KHZ=${cpu_frequency_khz} \
    KAPPA_USECS=${heartbeat_interval} \
    TARGET_POLLING_RATIO=${target_polling_count} \
    taskset -c 2 \
    make run_hbc >> ${output} ;
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

    for target_polling_count in ${target_polling_counts[@]} ; do
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} BEATS_STATS=true ACC=true CHUNKSIZE=1 &> /dev/null ;
      run_and_collect ${results}/${target_polling_count} ${target_polling_count} ;
    done

    clean ;
    cd ../ ;
  done
done

popd ;
