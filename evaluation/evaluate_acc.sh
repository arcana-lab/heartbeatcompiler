#!/bin/bash

# environment
ROOT_DIR=`git rev-parse --show-toplevel`
source ${ROOT_DIR}/enable ;
source common.sh ;

# experiment
experiment=acc
keyword=exectime
mkdir -p ${ROOT_DIR}/evaluation/results/${experiment};

########################################################
# experiment sections
baseline=true
minimal_sw=true     # mininal sliding window
maximal_sw=true     # maximal sliding window

# benchmark targetted
benchmarks=(mandelbrot spmv floyd_warshall plus_reduce_array srad)
########################################################

function run_and_collect {
  local technique=${1}
  local results_path=${2}
  mkdir -p ${results_path} ;
  local output=${results_path}/output.txt

  if [ ${technique} == "baseline" ] ; then
    for i in `seq 1 ${baseline_num_runs}` ; do
      taskset -c 0 make run_baseline >> ${output} ;
    done

  else
    for i in `seq 1 ${baseline_num_runs}` ; do
      WORKERS=${num_workers} \
      CPU_FREQUENCY_KHZ=${cpu_frequency_khz} \
      KAPPA_USECS=${heartbeat_interval} \
      make run_hbc >> ${output} ;
    done
  
  fi

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

    # minimal_sw
    if [ ${minimal_sw} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ACC=true CHUNKSIZE=1 ACC_MINIMAL=true &> /dev/null ;
      run_and_collect hbc ${results}/minimal_sw ;
    fi

    # maximal_sw
    if [ ${maximal_sw} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ACC=true CHUNKSIZE=1 ACC_MINIMAL=false &> /dev/null ;
      run_and_collect hbc ${results}/maximal_sw ;
    fi

    clean ;
    cd ../ ;
  done
done

popd ;
