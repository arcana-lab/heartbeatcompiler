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
hbc_acc=false     # software polling + acc
hbc_static=false  # software polling + static chunksize
hbc_rf=false      # rollforward + interrupt ping thread
hbc_rf_kmod=false # rollforward + kernel module
openmp=true

# if true, then track changes on element index
# otherwise, use array privatization
# default is false
array_update_map=${1}

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

    for i in `seq 1 ${baseline_num_runs}` ; do
      taskset -c 2 make run_baseline >> ${output} ;
    done

    # kill burnP6
    killall burnP6 &> /dev/null ;

  elif [ ${technique} == "openmp" ] ; then
    for i in `seq 1 10` ; do
      WORKERS=56 \
      timeout ${timeout} make run_openmp >> ${output} ;
    done

  else
    for i in `seq 1 10` ; do
      WORKERS=56 \
      CPU_FREQUENCY_KHZ=${cpu_frequency_khz} \
      KAPPA_USECS=${heartbeat_interval} \
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

    # baseline
    if [ ${baseline} = true ] ; then
      clean ; make baseline INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} &> /dev/null ;
      run_and_collect baseline ${results}/baseline ;
    fi

    # hbc_acc
    if [ ${hbc_acc} = true ] ; then
      clean ; make hbm INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ACC=true CHUNKSIZE=1 ARRAY_UPDATE_MAP=${array_update_map} &> /dev/null ;
      run_and_collect hbm_acc ${results}/hbm_acc_${array_update_map} ;
    fi

    # hbc_static
    if [ ${hbc_static} = true ] ; then
      clean ; make hbm INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ARRAY_UPDATE_MAP=${array_update_map} &> /dev/null ;
      run_and_collect hbm_static ${results}/hbm_static_${array_update_map} ;
    fi

    # hbc_rf
    if [ ${hbc_rf} = true ] ; then
      clean ; make hbm INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_ROLLFORWARD=true CHUNK_LOOP_ITERATIONS=false ARRAY_UPDATE_MAP=${array_update_map} &> /dev/null ;
      run_and_collect hbm_rf ${results}/hbm_rf_${array_update_map} ;
    fi

    # # hbc_rf_kmod
    # if [ ${hbc_rf_kmod} = true ] ; then
    #   clean ; make hbc INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_ROLLFORWARD=true USE_HB_KMOD=true CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
    #   run_and_collect hbc_rf_kmod ${results}/hbc_rf_kmod ;
    # fi

    # openmp
    if [ ${openmp} = true ] ; then
      omp_schedules=(STATIC DYNAMIC GUIDED)
      omp_nested_scheduling=(false)
      for omp_schedule in ${omp_schedules[@]} ; do
        for enable_omp_nested_scheduling in ${omp_nested_scheduling[@]} ; do
          clean ; make openmp INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} OMP_SCHEDULE=${omp_schedule} OMP_NESTED_SCHEDULING=${enable_omp_nested_scheduling} &> /dev/null ;
          run_and_collect openmp ${results}/openmp_`echo -e ${omp_schedule} | tr '[:upper:]' '[:lower:]'`_${enable_omp_nested_scheduling} ;
        done
      done
    fi

    clean ;
    cd ../ ;
  done
done

popd ;
