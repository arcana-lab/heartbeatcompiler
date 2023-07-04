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
hbc=true

# benchmark targetted
benchmarks=(mandelbrot mandelbulb cg spmv floyd_warshall plus_reduce_array srad)
########################################################

function run_and_collect {
  technique=${1}
  results_path=${2}
  mkdir -p ${results_path} ;
  output=${results_path}/output.txt

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
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_HEARTBEAT_PROMOTION=false &> /dev/null ;
      run_and_collect hbc ${results}/hbc ;
    fi

    clean ;
    cd ../ ;
  done
done

popd ;
