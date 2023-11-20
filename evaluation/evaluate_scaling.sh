#!/bin/bash

# environment
ROOT_DIR=`git rev-parse --show-toplevel`
source ${ROOT_DIR}/enable ;
source /project/extra/burnCPU/enable ;
source common.sh ;

# experiment
experiment=scaling
keyword=exectime
mkdir -p ${ROOT_DIR}/evaluation/results/${experiment};

########################################################
# experiment sections
baseline=false
hbc_acc=false                       # software polling + acc
hbc_static=false                    # software polling + static chunksize
hbc_rf=false                        # rollforward + interrupt ping thread
hbc_rf_kmod=false                   # rollforward + kernel module
openmp=false                        # default openmp with different schedulers
openmp_nested_parallelism=false     # run openmp with nested parallelism enabled
openmp_chunksize_sensitivity=false  # run openmp with various chunksize

# benchmark targetted
benchmarks=(cg floyd_warshall kmeans mandelbrot mandelbulb plus_reduce_array spmv srad)

# spmv settings
input_classes=(ARROWHEAD POWERLAW RANDOM)

# openmp settings
omp_schedules=(STATIC DYNAMIC GUIDED)
omp_nested_parallelism=(false true)
omp_chunksizes=(0 1 2 4 8 16)  # 0 means default chunksize
########################################################

function run_and_collect {
  local technique=${1}
  local results_path=${2}
  mkdir -p ${results_path} ;
  local output=${results_path}/output.txt

  if [ ${technique} == "baseline" ] ; then
    # burnP6
    killall burnP6 &> /dev/null ;

    for coreID in `seq -s' ' 4 2 10` ; do
      taskset -c ${coreID} burnP6 &
    done

    for i in `seq 1 3` ; do
      taskset -c 2 make run_baseline >> ${output} ;
    done

    # kill burnP6
    killall burnP6 &> /dev/null ;

  elif [ ${technique} == "openmp" ] ; then
    for i in `seq 1 5` ; do
      WORKERS=28 \
      numactl --physcpubind=28-55 --interleave=all \
      make run_openmp >> ${output} ;
    done

  else
    for i in `seq 1 5` ; do
      WORKERS=28 \
      CPU_FREQUENCY_KHZ=${cpu_frequency_khz} \
      KAPPA_USECS=${heartbeat_interval} \
      numactl --physcpubind=28-55 --interleave=all \
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
  if [ ${benchmark} != spmv ] ; then
    input_classes=(DEFAULT_INPUT_CLASS)
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

    # hbc_acc
    if [ ${hbc_acc} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ACC=true CHUNKSIZE=1 &> /dev/null ;
      run_and_collect hbc_acc ${results}/hbc_acc ;
    fi

    # hbc_static
    if [ ${hbc_static} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} &> /dev/null ;
      run_and_collect hbc_static ${results}/hbc_static ;
    fi

    # hbc_rf
    if [ ${hbc_rf} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_ROLLFORWARD=true CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
      run_and_collect hbc_rf ${results}/hbc_rf ;
    fi

    # hbc_rf_kmod
    if [ ${hbc_rf_kmod} = true ] ; then
      clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_ROLLFORWARD=true USE_HB_KMOD=true CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
      run_and_collect hbc_rf_kmod ${results}/hbc_rf_kmod ;
    fi

    # openmp
    if [ ${openmp} = true ] ; then
      for omp_schedule in ${omp_schedules[@]} ; do
        clean ; make openmp INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} OMP_SCHEDULE=${omp_schedule} &> /dev/null ;
        run_and_collect openmp ${results}/openmp_`echo -e ${omp_schedule} | tr '[:upper:]' '[:lower:]'` ;
      done
    fi

    # openmp_nested_parallelism
    if [ ${openmp_nested_parallelism} = true ] ; then
      for omp_schedule in ${omp_schedules[@]} ; do
        clean ; make openmp INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} OMP_SCHEDULE=${omp_schedule} OMP_NESTED_PARALLELISM=true &> /dev/null ;
        run_and_collect openmp ${results}/openmp_`echo -e ${omp_schedule} | tr '[:upper:]' '[:lower:]'`_nested_parallelism ;
      done
    fi

    # openmp_chunksize_sensitivity
    if [ ${openmp_chunksize_sensitivity} = true ] ; then
      for omp_schedule in ${omp_schedules[@]} ; do
        for omp_chunksize in ${omp_chunksizes[@]} ; do
          clean ; make openmp INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} OMP_SCHEDULE=${omp_schedule} OMP_CHUNKSIZE=${omp_chunksize} &> /dev/null ;
          run_and_collect openmp ${results}/openmp_`echo -e ${omp_schedule} | tr '[:upper:]' '[:lower:]'`_${omp_chunksize} ;
        done
      done
    fi

    clean ;
    cd ../ ;
  done
done

popd ;
