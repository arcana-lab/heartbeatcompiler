#!/bin/bash

# This script collects the time results for scaling plots
# 1. compile the baseline binary and runs the baseline binary to collect time results
# 2. compile the opencilk, openmp binary and runs across different core configurations
# 3. compile the heartbeat binary and runs across different core configurations
# 4. the time and output results are saved

bench=${1}
size=${2}
class=${3}
chunksize_0=${4}
chunksize_1=${5}

bench=floyd_warshall_`echo ${class} | tr '[:upper:]' '[:lower:]'`
results="../../results"
metric=time
warmup_secs=10.0
runs=10
keyword="exectime"

cd /nfs-scratch/yso0488/heartbeatcompiler0/benchmarks/floyd_warshall ;
mkdir -p ${results} ;

source /project/extra/burnCPU/enable
source /project/extra/llvm/14.0.6/enable
source /nfs-scratch/yso0488/jemalloc/enable

function run_baseline {
  impl="$1" ;
  path="$2" ;
  tmp=`mktemp`

  killall burnP6 &> /dev/null ;

  for coreID in `seq -s' ' 4 2 10` ; do
    taskset -c ${coreID} burnP6 &
  done

  TASKPARTS_NUM_WORKERS=1 \
  TASKPARTS_BENCHMARK_WARMUP_SECS=${warmup_secs} \
  TASKPARTS_BENCHMARK_VERBOSE=1 \
  TASKPARTS_BENCHMARK_NUM_REPEAT=${runs} \
  make run_${impl} > ${tmp} 2>&1 ;
  cat ${tmp} | grep ${keyword} | awk '{ print $2 }' | tr -d ',' >> ${path}/${metric}1.txt ;
  cat ${tmp} >> ${path}/output1.txt ;

  killall burnP6 &> /dev/null ;

  rm ${tmp} ;
}

function run_parallel {
  impl="$1" ;
  path="$2" ;
  tmp=`mktemp`

  killall burnP6 &> /dev/null ;

  for coreID in `seq -s' ' 4 2 10` ; do
    taskset -c ${coreID} burnP6 &
  done

  TASKPARTS_NUM_WORKERS=1 \
  TASKPARTS_BENCHMARK_WARMUP_SECS=${warmup_secs} \
  TASKPARTS_BENCHMARK_VERBOSE=1 \
  TASKPARTS_BENCHMARK_NUM_REPEAT=${runs} \
  make run_${impl} > ${tmp} 2>&1 ;
  cat ${tmp} | grep ${keyword} | awk '{ print $2 }' | tr -d ',' >> ${path}/${metric}1.txt ;
  cat ${tmp} >> ${path}/output1.txt ;

  killall burnP6 &> /dev/null ;

  TASKPARTS_NUM_WORKERS=16 \
  TASKPARTS_BENCHMARK_WARMUP_SECS=${warmup_secs} \
  TASKPARTS_BENCHMARK_VERBOSE=1 \
  TASKPARTS_BENCHMARK_NUM_REPEAT=${runs} \
  make run_${impl} > ${tmp} 2>&1 ;
  cat ${tmp} | grep ${keyword} | awk '{ print $2 }' | tr -d ',' >> ${path}/${metric}16.txt ;
  cat ${tmp} >> ${path}/output16.txt ;

  rm ${tmp} ;
}

make clean &> /dev/null ;
killall burnP6 &> /dev/null ;


make clean ; make baseline INPUT_CLASS=${class} INPUT_SIZE=${size} ;
folder="${results}/${metric}/${bench}/${size}/baseline"
mkdir -p ${folder} ;
run_baseline "baseline" ${folder} ;


killall burnP6 &> /dev/null ;
make clean &> /dev/null ;
