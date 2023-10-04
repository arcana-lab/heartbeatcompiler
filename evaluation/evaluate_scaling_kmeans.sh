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
baseline=true
hbc_acc=true     # software polling + acc
hbc_static=false  # software polling + static chunksize
hbc_rf=true      # rollforward + interrupt ping thread
hbc_rf_kmod=false # rollforward + kernel module
openmp=true

# benchmark targetted
benchmarks=(kmeans)
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

    for i in `seq 1 5` ; do
      taskset -c 2 make run_baseline >> ${output} ;
    done

    # kill burnP6
    killall burnP6 &> /dev/null ;

  elif [ ${technique} == "openmp" ] ; then
    for i in `seq 1 10` ; do
      WORKERS=56 \
      timeout ${timeout} \
      numactl --physcpubind=0-55 --interleave=all \
      make run_openmp >> ${output} ;
    done

  else
    for i in `seq 1 10` ; do
      WORKERS=56 \
      CPU_FREQUENCY_KHZ=${cpu_frequency_khz} \
      KAPPA_USECS=${heartbeat_interval} \
      numactl --physcpubind=0-55 --interleave=all \
      make run_hbm >> ${output} ;
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

  results=${ROOT_DIR}/evaluation/results/${experiment}/${benchmark}
  mkdir -p ${results} ;

  cd ${benchmark} ;
  clean ;

  # baseline
  if [ ${baseline} = true ] ; then
    clean ; make baseline INPUT_SIZE=${input_size} &> /dev/null ;
    run_and_collect baseline ${results}/baseline ;
  fi

  # hbc_acc
  if [ ${hbc_acc} = true ] ; then
    clean ; make hbm INPUT_SIZE=${input_size} ACC=true CHUNKSIZE=1 &> /dev/null ;
    run_and_collect hbm_acc ${results}/hbm_acc ;
  fi

  # hbc_rf
  if [ ${hbc_rf} = true ] ; then
    clean ; make hbm INPUT_SIZE=${input_size} ENABLE_ROLLFORWARD=true CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
    run_and_collect hbm_rf ${results}/hbm_rf ;
  fi

  # openmp
  if [ ${openmp} = true ] ; then
    omp_schedules=(STATIC DYNAMIC GUIDED)
    for omp_schedule in ${omp_schedules[@]} ; do
      clean ; make openmp INPUT_SIZE=${input_size} OMP_SCHEDULE=${omp_schedule} &> /dev/null ;
      run_and_collect openmp ${results}/openmp_`echo -e ${omp_schedule} | tr '[:upper:]' '[:lower:]'` ;
    done
  fi

  clean ;
  cd ../ ;

done

popd ;
