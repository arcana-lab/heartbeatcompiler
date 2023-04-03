#!/bin/bash
# set -o xtrace       # outputs the script's progress
ROOT_DIR=`git rev-parse --show-toplevel`

# experiment settings
experiment=heartbeat_rate
# benchmarks=('floyd_warshall' 'kmeans' 'mandelbrot' 'plus_reduce_array' 'spmv' 'srad')
benchmarks=('plus_reduce_array')
input_size=tpal
heartbeat_rates=(2 10 30 100 1000 10000 100000)
metric=time
keyword="exectime"

# runtime settings
num_workers=16
warmup_secs=3.0
runs=10
verbose=1

source /project/extra/llvm/9.0.0/enable
source ${ROOT_DIR}/noelle/enable
source /nfs-scratch/yso0488/jemalloc/enable

function run_testing {
  impl="$1"
  path="$2"
  tmp=`mktemp`

  for kappa_usec in "${heartbeat_rates[@]}" ; do
    TASKPARTS_KAPPA_USEC=${kappa_usec} \
    TASKPARTS_NUM_WORKERS=${num_workers} \
    TASKPARTS_BENCHMARK_WARMUP_SECS=${warmup_secs} \
    TASKPARTS_BENCHMARK_NUM_REPEAT=${runs} \
    TASKPARTS_BENCHMARK_VERBOSE=${verbose} \
    make run_${impl} > ${tmp} 2>&1 ;
    cat ${tmp} | grep ${keyword} | awk '{ print $2 }' | tr -d ',' >> ${path}/${metric}${kappa_usec}.txt ;
    cat ${tmp} >> ${path}/output${kappa_usec}.txt ;
  done

  rm -rf ${tmp} ;
}

results=${ROOT_DIR}/results
mkdir -p ${results} ;

pushd . ;

cd ${ROOT_DIR}/benchmarks ;
make link &> /dev/null ;

for bench in "${benchmarks[@]}" ; do
  cd ${bench} ;

  make clean &> /dev/null ;
  INPUT_SIZE=${input_size} ENABLE_ROLLFORWARD=false make heartbeat_compiler ;
  folder="${results}/${experiment}/${bench}/${input_size}/heartbeat_compiler_software_polling"
  mkdir -p ${folder} ;
  run_testing "heartbeat_compiler" ${folder} ;
  make clean &> /dev/null ;

  make clean &> /dev/null ;
  INPUT_SIZE=${input_size} ENABLE_ROLLFORWARD=true make heartbeat_compiler ;
  folder="${results}/${experiment}/${bench}/${input_size}/heartbeat_compiler_rollforward"
  mkdir -p ${folder} ;
  run_testing "heartbeat_compiler" ${folder} ;
  make clean &> /dev/null ;

  cd ../ ;
done

popd ;
