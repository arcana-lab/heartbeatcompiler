#!/bin/bash

# environment
ROOT_DIR=`git rev-parse --show-toplevel`
source ${ROOT_DIR}/enable ;
source common.sh

# experiment
experiment=acc_justify
keyword=exectime
mkdir -p ${ROOT_DIR}/evaluation/results/${experiment} ;

########################################################
# experiment sections
chunking=true
  chunking_baseline=true
  chunking_hbc=true

composed=true
  composed_baseline=true
  composed_hbc=true
  composed_hbc_acc=true


# benchmark targetted
benchmark=mandelbrot
inputs=(2000_1000000_1 10_20_10000000)
chunksizes=(1 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 52428800 1048577)
########################################################

function run_input_and_collect {
  local technique=${1}
  local results_path=${2}
  local input=${3}
  mkdir -p ${results_path} ;
  local output=${results_path}/output.txt

  if [ ${technique} == "baseline" ] ; then
    for i in `seq 1 ${baseline_num_runs}` ; do
      ARGS="`echo ${input} | tr '_' ' '`" \
      taskset -c 0 make run_baseline >> ${output} ;
    done
  else
    for i in `seq 1 ${baseline_num_runs}` ; do
      WORKERS=${num_workers} \
      CPU_FREQUENCY_KHZ=${cpu_frequency_khz} \
      KAPPA_USECS=${heartbeat_interval} \
      ARGS="`echo ${input} | tr '_' ' '`" \
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
results=${ROOT_DIR}/evaluation/results/${experiment}/${benchmark}
mkdir -p ${results} ;

cd ${benchmark} ;
clean ;

# chunking
if [ ${chunking} = true ] ; then
  for input in ${inputs[@]} ; do
    mkdir -p ${results}/${input} ;

    # baseline
    if [ ${chunking_baseline} = true ] ; then
      clean ; make baseline INPUT_SIZE=user ACC_JUSTIFY_CHUNKING=true &> /dev/null ;
      run_input_and_collect baseline ${results}/${input}/baseline ${input} ;
    fi

    if [ ${chunking_hbc} = true ] ; then
      for chunksize in ${chunksizes[@]} ; do
        clean ; make hbc INPUT_SIZE=user ACC_JUSTIFY_CHUNKING=true CHUNKSIZE=${chunksize} &> /dev/null ;
        run_input_and_collect hbc ${results}/${input}/hbc/${chunksize} ${input} ;
      done
    fi
  done
fi

# composed
mkdir -p ${results}/composed ;

if [ ${composed_baseline} = true ] ; then
  clean ; make baseline ACC_JUSTIFY_COMPOSED=true &> /dev/null ;
  run_input_and_collect baseline ${results}/composed/baseline composed ;
fi

if [ ${composed_hbc} = true ] ; then
  for chunksize in ${chunksizes[@]} ; do
    clean ; make hbc ACC_JUSTIFY_COMPOSED=true CHUNKSIZE=${chunksize} &> /dev/null ;
    run_input_and_collect hbc ${results}/composed/hbc/${chunksize} composed ;
  done
fi

if [ ${composed_hbc_acc} = true ] ; then
  for chunksize in ${chunksizes[@]} ; do
    clean ; make hbc ACC_JUSTIFY_COMPOSED=true ACC=true CHUNKSIZE=${chunksize} &> /dev/null ;
    run_input_and_collect hbc_acc ${results}/composed/hbc_acc/${chunksize} composed ;
  done
fi

clean ;
cd ../ ;

popd ;
