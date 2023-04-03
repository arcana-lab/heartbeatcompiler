set -o xtrace       # outputs the script's progress

#!/bin/bash

# run this script in the benchmarks/ directory

benchmarks=('floyd_warshall' 'kmeans' 'mandelbrot' 'nested_plus_reduce_array' 'plus_reduce_array' 'spmv' 'srad')
hb_rates=(2 10 30 100 1000 10000 100000)

results="../../results"
metric=time
size=testing
warmup_secs=3.0
runs=10
keyword="exectime"

mkdir -p ${results} ;

source /project/extra/burnCPU/enable
source /project/extra/llvm/9.0.0/enable
source /nfs-scratch/jxl7125/heartbeatcompiler/noelle/enable
source /nfs-scratch/yso0488/jemalloc/enable

function run_testing {
  kappa="$1" ;
  path="$2" ;
  tmp=`mktemp`

  TASKPARTS_NUM_WORKERS=1 \
  TASKPARTS_BENCHMARK_WARMUP_SECS=${warmup_secs} \
  TASKPARTS_BENCHMARK_VERBOSE=1 \
  TASKPARTS_BENCHMARK_NUM_REPEAT=${runs} \
  TASKPARTS_KAPPA_USEC=${kappa} \
  make run_hbc > ${tmp} 2>&1 ;
  rm -f ${path}/${metric}${kappa}.txt ;
  rm -f ${path}/output${kappa}.txt ;
  cat ${tmp} | grep ${keyword} | awk '{ print $2 }' | tr -d ',' >> ${path}/${metric}${kappa}.txt ;
  cat ${tmp} >> ${path}/output${kappa}.txt ;
  rm ${tmp} ;
}

for bench in "${benchmarks[@]}" ; do
    cd ${bench} ;
    make clean ; ENABLE_ROLLFORWARD=false make hbc ;
    folder="${results}/${metric}/${bench}/${size}/heartbeat_compiler_hb_rate" ;
    mkdir -p ${folder} ;
    for kappa in "${hb_rates[@]}" ; do
        run_testing ${kappa} ${folder} ;
    done
    cd .. ;
done

make clean &> /dev/null ;