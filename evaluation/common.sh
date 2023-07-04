#!/bin/bash

# global settings
input_size=benchmarking
baseline_num_runs=10
num_runs=100
num_workers=64
cpu_frequency_khz=3000000
heartbeat_interval=100 # microseconds
timeout=30s

# utility functions
function clean {
  make clean &> /dev/null ;
}

function collect {
  results_path=${1}
  input=${2}
  output=${results_path}/${keyword}.txt

  cat ${input} | grep -w ${keyword} | awk '{ print $2 }' | tr -d '},' >> ${output} ;
}
