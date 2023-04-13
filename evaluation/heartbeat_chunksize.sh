#!/bin/bash

# This evaluation script does the following things
# 1. compile the heartbeat binary of each benchmark (including various input classes) using various chunksizes (and no chunking)
# 2. using rollforward and software_polling as signaling mechanism separately
# 3. run the heartbeat binary
# 4. collect the time result

# source environment
ROOT_DIR=`git rev-parse --show-toplevel`
source ${ROOT_DIR}/enable

# experiment settings
experiment=chunksize
benchmarks=('floyd_warshall' 'kmeans' 'mandelbrot' 'plus_reduce_array' 'spmv' 'srad')
chunksizes=(1 10 100 1000 10000)
input_size=tpal
signal_mechanisms=('rollforward' 'rollforward_kmod' 'software_polling')
metric=time
keyword="exectime"

# runtime settings
num_worker=1
heartbeat_rate=100
warmup_secs=10.0
runs=10
verbose=1

function evaluate_benchmark {
  bench_name=$1
  input_class=$2
  echo -e "benchmark: " ${bench_name} ;

  for signal_mechanism in ${signal_mechanisms[@]} ; do
    echo -e "\tsignal_mechanism: " ${signal_mechanism} ;

    if [ ${signal_mechanism} == 'rollforward' ] ; then
      enable_rollforward=true
      use_hb_kmod=false
    elif [ ${signal_mechanism} == 'rollforward_kmod' ] ; then
      enable_rollforward=true
      use_hb_kmod=true
    else
      enable_rollforward=false
      use_hb_kmod=false
    fi

    if [[ ${bench_name} == 'kmeans' || ${bench_name} == 'plus_reduce_array' ]] ; then
      level='0'
      prefix=''
    else
      level='1'
      prefix='1_'
    fi

    # generate the path to store results
    result_path=${results}/${experiment}/${bench_name}/${input_size}/heartbeat_compiler_${signal_mechanism}
    mkdir -p ${result_path} ;
    echo -e "\tresult path: " ${result_path} ;

    # compile the benchmark (baseline without chunking)
    make clean &> /dev/null ;
    INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_ROLLFORWARD=${enable_rollforward} USE_HB_KMOD=${use_hb_kmod} CHUNK_LOOP_ITERATIONS=false make heartbeat_compiler &> /dev/null ;

    # run the benchmark binary (baseline without chunking)
    run_experiment "heartbeat_compiler" ${result_path} '0' ;

    for chunksize in ${chunksizes[@]} ; do
      # compile the benchmark
      make clean &> /dev/null ;
      INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_ROLLFORWARD=${enable_rollforward} USE_HB_KMOD=${use_hb_kmod} CHUNKSIZE_${level}=${chunksize} make heartbeat_compiler &> /dev/null ;

      # run the benchmark binary
      run_experiment "heartbeat_compiler" ${result_path} "${prefix}${chunksize}" ;
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

  TASKPARTS_NUM_WORKERS=${num_worker} \
  TASKPARTS_KAPPA_USEC=${heartbeat_rate} \
  TASKPARTS_BENCHMARK_WARMUP_SECS=${warmup_secs} \
  TASKPARTS_BENCHMARK_NUM_REPEAT=${runs} \
  TASKPARTS_BENCHMARK_VERBOSE=${verbose} \
  make run_${implementation} > ${tmp} 2>&1 ;
  cat ${tmp} | grep ${keyword} | awk '{ print $2 }' | tr -d ',' >> ${result_path}/${metric}_${config}.txt ;
  cat ${tmp} >> ${result_path}/output_${config}.txt ;

  rm -rf ${tmp} ;
}

# directory to store the final results
results=${ROOT_DIR}/evaluation/results
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