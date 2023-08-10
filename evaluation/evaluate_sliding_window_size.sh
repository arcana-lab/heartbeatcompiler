#!/bin/bash

# environment
ROOT_DIR=`git rev-parse --show-toplevel`
source ${ROOT_DIR}/enable ;
source common.sh ;

# experiment
experiment=sliding_window_size
keyword=exectime
mkdir -p ${ROOT_DIR}/evaluation/results/${experiment};

########################################################
# experiment sections
sliding_window_sizes=(5 10 25 50 100 250 500)

# benchmark targetted
benchmarks=(mandelbrot spmv floyd_warshall plus_reduce_array srad)
########################################################

function run_and_collect {
  local results_path=${1}
  local sliding_window_size=${2}
  mkdir -p ${results_path} ;
  local output=${results_path}/output.txt

  for i in `seq 1 3` ; do
    WORKERS=1 \
    CPU_FREQUENCY_KHZ=${cpu_frequency_khz} \
    KAPPA_USECS=${heartbeat_interval} \
    SLIDING_WINDOW_SIZE=${sliding_window_size} \
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

    for sliding_window_size in ${sliding_window_sizes[@]} ; do
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ACC=true CHUNKSIZE=1 &> /dev/null ;
      run_and_collect ${results}/${sliding_window_size} ${sliding_window_size} ;
    done

    clean ;
    cd ../ ;
  done
done

popd ;
