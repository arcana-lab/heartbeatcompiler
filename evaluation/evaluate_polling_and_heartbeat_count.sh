#!/bin/bash

# environment
ROOT_DIR=`git rev-parse --show-toplevel`
source ${ROOT_DIR}/enable ;
source common.sh

# experiment
experiment=poll_and_heartbeat_counts
keyword=null
results=${ROOT_DIR}/evaluation/results/${experiment}/${benchmark}
mkdir -p ${results} ;

########################################################
# benchmark targetted
benchmark=spmv
input_size=tpal
input_class=POWERLAW
chunksizes=(1 2 4 8 16 32 64 128 256 512 1024)
########################################################

function run_and_collect {
  local technique=${1}
  local results_path=${2}
  mkdir -p ${results_path} ;
  local output=${results_path}/output.txt

  for chunksize in ${chunksizes[@]} ; do
    for i in `seq 1 1` ; do
      WORKERS=1 \
      CPU_FREQUENCY_KHZ=${cpu_frequency_khz} \
      KAPPA_USECS=${heartbeat_interval} \
      CHUNKSIZE=${chunksize} \
      taskset -c 2 \
      make run_hbc >> ${output} ;
    done
  done

  keyword="wasted_polls"
  collect ${results_path} ${output} ;

  keyword="detection_rate"
  collect ${results_path} ${output} ;
}

# main script
pushd . ;

# preparation
cd ${ROOT_DIR}/benchmarks ;
make link &> /dev/null ;

cd ${benchmark} ;
clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} BEATS_STATS=true &> /dev/null ;
run_and_collect hbc ${results}/`echo -e ${input_class} | tr '[:upper:]' '[:lower:]'` ;

clean ;
cd ../ ;

popd ;
