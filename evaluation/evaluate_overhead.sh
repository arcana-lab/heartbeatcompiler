#!/bin/bash

# environment
ROOT_DIR=`git rev-parse --show-toplevel`
source ${ROOT_DIR}/enable ;
source common.sh ;

# experiment
experiment=overhead
keyword=exectime
mkdir -p ${ROOT_DIR}/evaluation/results/${experiment} ;

########################################################
# experiment sections
baseline=false

sp_baseline=false          # overhead of outlining using software polling
sp_environment=false       # overhead of preparing environment using software polling
sp_loop_chunking=false     # overhead of loop chunking transformation (run with static chunksize)
sp_promotion_insertion=false # overhead of promotion handler call and any logic added 
sp_chunk_transfer=false    # overhead of chunksize transferring (this cannot be measured with ac, since ac relies on sp to adjust chunksize)
sp_no_chunking=false       # overhead of polling without loop chunking
sp_static_chunking=true   # overhead of polling with loop chunking
sp_aca=false               # overhead of polling with adaptive chunksize adjustment (including both chunk transfer, ac and polling cost)
sp_scheduling=false        # overhead of scheduling based upon adaptive chunksize adjustment

rf_baseline=false          # overhead of outlining using rollforwarding
rf_environment=false       # overhead of preparing environment using rollforwarding
rf_cost=false              # overhead of rollforwarding
rf_scheduling=false        # overhead of shceduling using rollforwarding

rf_kmod_baseline=false     # overhead of outlining using rollforwarding via kernel module
rf_kmod_environment=false  # overhead of preparing environment using rollforwarding via kernel module
rf_kmod_cost=false         # overhead of rollforwarding via kernel module
rf_kmod_scheduling=false   # overhead of scheduling using rollforwarding via kernel module

# benchmark targetted
benchmarks=(floyd_warshall srad)

# implementation to use, either hbc or hbm
impl=hbm
if [ ${1} ] ; then
  impl=${1}
fi
########################################################

function run_and_collect {
  local technique=${1}
  local results_path=${2}
  mkdir -p ${results_path} ;
  local output=${results_path}/output.txt

  for i in `seq 1 10` ; do
    WORKERS=1 taskset -c 2 make run_${technique} >> ${output} ;
  done

  collect ${results_path} ${output} ;
}

function run_and_collect_polls {
  local technique=${1}
  local results_path=${2}
  mkdir -p ${results_path} ;
  local output=${results_path}/output.txt

  for i in `seq 1 10` ; do
    WORKERS=1 taskset -c 2 make run_${technique} >> ${output} ;
  done

  keyword=exectime
  collect ${results_path} ${output} ;

  keyword=polls
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

    # sp_baseline
    if [ ${sp_baseline} = true ] ; then
      clean ; make ${impl} INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=false &> /dev/null ;
      run_and_collect ${impl} ${results}/sp_baseline ;
    fi

    # sp_environment
    if [ ${sp_environment} = true ] ; then
      clean ; make ${impl} INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_HEARTBEAT=false CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
      run_and_collect ${impl} ${results}/sp_environment ;
    fi

    # sp_loop_chunking
    if [ ${sp_loop_chunking} = true ] ; then
      clean ; make ${impl} INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_HEARTBEAT=false CHUNK_LOOP_ITERATIONS=true &> /dev/null ;
      run_and_collect ${impl} ${results}/sp_loop_chunking ;
    fi

    # sp_promotion_insertion
    if [ ${sp_promotion_insertion} = true ] ; then
      clean ; make ${impl} INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_HEARTBEAT=true CHUNK_LOOP_ITERATIONS=true PROMOTION_INSERTION_OVERHEAD_ANALYSIS=true ENABLE_PROMOTION=false &> /dev/null ;
      run_and_collect ${impl} ${results}/sp_promotion_insertion ;
    fi

    # sp_chunk_transfer
    if [ ${sp_chunk_transfer} = true ] ; then
      clean ; make ${impl} INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} CHUNKSIZE_TRANSFERRING_OVERHEAD_ANALYSIS=true &> /dev/null ;
      run_and_collect ${impl} ${results}/sp_chunk_transfer ;
    fi

    # sp_no_chunking
    if [ ${sp_no_chunking} = true ] ; then
      clean ; make ${impl} INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_PROMOTION=false CHUNK_LOOP_ITERATIONS=false OVERHEAD_ANALYSIS=true &> /dev/null ;
      run_and_collect ${impl} ${results}/sp_no_chunking ;
    fi

    # sp_static_chunking
    if [ ${sp_static_chunking} = true ] ; then
      clean ; make ${impl} INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_PROMOTION=false OVERHEAD_ANALYSIS=true POLLS_STATS=true &> /dev/null ;
      run_and_collect_polls ${impl} ${results}/sp_static_chunking ;
    fi

    # sp_aca
    if [ ${sp_aca} = true ] ; then
      clean ; make ${impl} INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_PROMOTION=false OVERHEAD_ANALYSIS=true ACC=true CHUNKSIZE=1 POLLS_STATS=true &> /dev/null ;
      run_and_collect_polls ${impl} ${results}/sp_aca ;
    fi

    # sp_scheduling
    if [ ${sp_scheduling} = true ] ; then
      clean ; make ${impl} INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ACC=true CHUNKSIZE=1 &> /dev/null ;
      run_and_collect ${impl} ${results}/sp_scheduling ;
    fi

    # rf_baseline
    if [ ${rf_baseline} = true ] ; then
      clean ; make ${impl} INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=false ENABLE_ROLLFORWARD=true &> /dev/null ;
      run_and_collect ${impl} ${results}/rf_baseline ;
    fi

    # rf_environment
    if [ ${rf_environment} = true ] ; then
      clean ; make ${impl} INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=true ENABLE_HEARTBEAT=false ENABLE_ROLLFORWARD=true CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
      run_and_collect ${impl} ${results}/rf_environment ;
    fi

    # rf_cost
    if [ ${rf_cost} = true ] ; then
      clean ; make ${impl} INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=true ENABLE_HEARTBEAT=true ENABLE_PROMOTION=false ENABLE_ROLLFORWARD=true CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
      run_and_collect ${impl} ${results}/rf_cost ;
    fi

    # rf_scheduling
    if [ ${rf_scheduling} = true ] ; then
      clean ; make ${impl} INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=true ENABLE_HEARTBEAT=true ENABLE_PROMOTION=true ENABLE_ROLLFORWARD=true CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
      run_and_collect ${impl} ${results}/rf_scheduling ;
    fi

    # rf_kmod_baseline
    if [ ${rf_kmod_baseline} = true ] ; then
      clean ; make ${impl} INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=false ENABLE_ROLLFORWARD=true USE_HB_KMOD=true &> /dev/null ;
      run_and_collect ${impl} ${results}/rf_kmod_baseline ;
    fi

    # rf_kmod_environment
    if [ ${rf_kmod_environment} = true ] ; then
      clean ; make ${impl} INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=true ENABLE_HEARTBEAT=false ENABLE_ROLLFORWARD=true USE_HB_KMOD=true CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
      run_and_collect ${impl} ${results}/rf_kmod_environment ;
    fi

    # rf_kmod_cost
    if [ ${rf_kmod_cost} = true ] ; then
      clean ; make ${impl} INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} ENABLE_PROMOTION=false ENABLE_ROLLFORWARD=true USE_HB_KMOD=true CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
      run_and_collect ${impl} ${results}/rf_kmod_cost ;
    fi

    # rf_kmod_scheduling
    if [ ${rf_kmod_scheduling} = true ] ; then
      clean ; make ${impl} INPUT_SIZE=${input_size} INPUT_CLASS=${input_class} RUN_HEARTBEAT=true ENABLE_HEARTBEAT=true ENABLE_PROMOTION=true ENABLE_ROLLFORWARD=true USE_HB_KMOD=true CHUNK_LOOP_ITERATIONS=false &> /dev/null ;
      run_and_collect ${impl} ${results}/rf_kmod_scheduling ;
    fi

    clean ;
    cd ../ ;
  done
done

popd ;
