#!/bin/bash

# environment
ROOT_DIR=`git rev-parse --show-toplevel`
source ${ROOT_DIR}/enable ;
source common.sh ;

# experiment
experiment=target_polling_count
mkdir -p ${ROOT_DIR}/evaluation/results/${experiment};

########################################################
# experiment sections
target_polling_counts=(1 2 4 8 16 32)
# target_polling_counts=(1 10 100 1000 10000 100000 1000000) # after a certain time, the chunksize will be 1 and no longer shrink

detection_rate=true
speedup=false

# benchmark targetted
benchmarks=(plus_reduce_array)
########################################################

function run_and_collect_detection_rate {
  local results_path=${1}
  local target_polling_count=${2}
  mkdir -p ${results_path} ;
  local output=${results_path}/detection_rate_output.txt

  for i in `seq 1 3` ; do
    WORKERS=1 \
    CPU_FREQUENCY_KHZ=${cpu_frequency_khz} \
    KAPPA_USECS=${heartbeat_interval} \
    TARGET_POLLING_RATIO=${target_polling_count} \
    taskset -c 2 \
    make run_hbc >> ${output} ;
  done

  keyword=detection_rate
  collect ${results_path} ${output} ;

  keyword=wasted_polls
  collect ${results_path} ${output} ;
}

function run_and_collect_exectime {
  local results_path=${1}
  local target_polling_count=${2}
  mkdir -p ${results_path} ;
  local output=${results_path}/speedup_output.txt

  for i in `seq 1 3` ; do
    WORKERS=64 \
    CPU_FREQUENCY_KHZ=${cpu_frequency_khz} \
    KAPPA_USECS=${heartbeat_interval} \
    TARGET_POLLING_RATIO=${target_polling_count} \
    make run_hbc >> ${output} ;
  done

  keyword=exectime
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

      # detection_rate
      if [ ${detection_rate} = true ] ; then
        clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} BEATS_STATS=true SPMV_DETECTION_RATE_ANALYSIS=true ACC=true CHUNKSIZE=1 &> /dev/null ;
        run_and_collect_detection_rate ${results}/${target_polling_count} ${target_polling_count} ;
      fi

      # speedup
      if [ ${speedup} = true ] ; then
        clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ACC=true CHUNKSIZE=1 &> /dev/null ;
        run_and_collect_exectime ${results}/${target_polling_count} ${target_polling_count} ;
      fi

    done

    clean ;
    cd ../ ;
  done
done

popd ;
