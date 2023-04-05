#!/bin/bash
# source environment
ROOT_DIR=`git rev-parse --show-toplevel`
source /project/extra/llvm/9.0.0/enable
source /nfs-scratch/yso0488/jemalloc/enable

# experiment settings
experiment=scaling
benchmarks=('floyd_warshall' 'kmeans' 'mandelbrot' 'plus_reduce_array' 'spmv' 'srad')
input_size=tpal
implementations=('openmp' 'opencilk')
metric=time
keyword="exectime"

# runtime settings
num_workers=(1 2 4 8 16 28 56)
warmup_secs=3.0
runs=10
verbose=1

function evaluate_benchmark {
  bench=$1
  input_class=$2
  echo -e "benchmark: " ${bench} ;

  for implementation in ${implementations[@]} ; do
    echo -e "\timplementation: " ${implementation} ;

    if [ ${implementation} == 'openmp' ] ; then
      env_variable="OMP_NUM_THREADS"
    elif [ ${implementation} == 'opencilk' ] ; then
      env_variable="CILK_NWORKERS"
    fi

    # compile the benchmark
    make clean &> /dev/null ;
    INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} make ${implementation} &> /dev/null ;
    
    # generate the path to store results
    result_path=${results}/${experiment}/${bench}/${input_size}/${implementation}
    mkdir -p ${result_path} ;
    echo -e "\tresult path: " ${result_path} ;

    # run the benchmark binary
    for num_worker in ${num_workers[@]} ; do
      run_experiment ${implementation} ${result_path} ${env_variable} ${num_worker} ;
    done

    make clean &> /dev/null ;
  done
}

function run_experiment {
  implementation=$1
  result_path=$2
  env_variable=$3
  config=$4
  tmp=`mktemp`
  echo -e "\t\tconfig: " ${config} ;

  export ${env_variable}=${config} ;
  UTILITY_BENCHMARK_WARMUP_SECS=${warmup_secs} \
  UTILITY_BENCHMARK_NUM_REPEAT=${runs} \
  UTILITY_BENCHMARK_VERBOSE=${verbose} \
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
