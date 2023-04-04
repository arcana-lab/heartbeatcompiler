#!/bin/bash
# source environment
ROOT_DIR=`git rev-parse --show-toplevel`
source /project/extra/llvm/9.0.0/enable
source ${ROOT_DIR}/noelle/enable
source /nfs-scratch/yso0488/jemalloc/enable

# experiment settings
experiment=heartbeat_rate
benchmarks=('floyd_warshall' 'kmeans' 'mandelbrot' 'plus_reduce_array' 'spmv' 'srad')
input_size=tpal
signal_mechanisms=('rollforward' 'software_polling')
heartbeat_rates=(2 10 30 100 1000 10000 100000)
metric=time
keyword="exectime"

# runtime settings
num_workers=16
warmup_secs=3.0
runs=10
verbose=1

function evaluate_benchmark {
  bench=$1
  input_class=$2
  echo -e "benchmark: " ${bench} ;

  for signal_mechanism in ${signal_mechanisms[@]} ; do
    echo -e "\tsignal_mechanism: " ${signal_mechanism} ;

    if [ ${signal_mechanism} == 'rollforward' ] ; then
      enable_rollforward=true
    else
      enable_rollforward=false
    fi

    # compile the benchmark
    # if we want to experiment various factors at the compile time, use for loop here
    make clean &> /dev/null ;
    INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_ROLLFORWARD=${enable_rollforward} make heartbeat_compiler &> /dev/null ;
    
    # generate the path to store results
    result_path=${results}/${experiment}/${bench}/${input_size}/heartbeat_compiler_${signal_mechanism}
    mkdir -p ${result_path} ;
    echo -e "\tresult path: " ${result_path} ;

    # run the benchmark binary
    # if want to experiment various factors at the runtime, use for loop here
    for heartbeat_rate in ${heartbeat_rates[@]} ; do
      run_experiment "heartbeat_compiler" ${result_path} ${heartbeat_rate} ;
    done

    make clean &> /dev/null ;
  done
}

function run_experiment {
  implementation=$1
  result_path=$2
  config=$3
  tmp=`mktemp`
  echo -e "\t\tconfig: " ${config} ;

  TASKPARTS_KAPPA_USEC=${config} \
  TASKPARTS_NUM_WORKERS=${num_workers} \
  TASKPARTS_BENCHMARK_WARMUP_SECS=${warmup_secs} \
  TASKPARTS_BENCHMARK_NUM_REPEAT=${runs} \
  TASKPARTS_BENCHMARK_VERBOSE=${verbose} \
  make run_${implementation} > ${tmp} 2>&1 ;
  cat ${tmp} | grep ${keyword} | awk '{ print $2 }' | tr -d ',' >> ${result_path}/${metric}_${config}.txt ;
  cat ${tmp} >> ${result_path}/output_${config}.txt ;

  rm -rf ${tmp} ;
}

# directory to store the final results
results=${ROOT_DIR}/results
mkdir -p ${results} ;

pushd . ;

# enter benchmarks directory and link common files
cd ${ROOT_DIR}/benchmarks ;
make link &> /dev/null ;

for bench in ${benchmarks[@]} ; do
  cd ${bench} ;

  # determine the list of input classes used by the benchmark
  if [ ${bench} == "floyd_warshall" ] ; then
    input_classes=('1K' '2K')
  elif [ ${bench} == "spmv" ] ; then
    input_classes=('RANDOM' 'POWERLAW' 'ARROWHEAD')
  else
    input_classes=()
  fi

  if [ ${#input_classes[@]} -gt 0 ] ; then
    for input_class in ${input_classes[@]} ; do
      evaluate_benchmark ${bench}_`echo -e ${input_class} | tr '[:upper:]' '[:lower:]'` ${input_class} ;
    done
  else
    evaluate_benchmark ${bench} '' ;
  fi

  cd ../ ;
done

popd ;
