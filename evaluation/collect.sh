#!/bin/bash

# environment
ROOT_DIR=`git rev-parse --show-toplevel`
source ${ROOT_DIR}/enable ;
source common.sh ;

function run_and_collect {
  local technique=${1}
  local results_path=${2}
  mkdir -p ${results_path} ;
  local output=${results_path}/output.txt

  keyword=polls
  collect ${results_path} ${output} ;
}

cd ${ROOT_DIR}/evaluation/results/overhead ;
results=${ROOT_DIR}/evaluation/results/overhead

for bench in `ls` ; do
  run_and_collect hbc ${results}/${bench}/sp_static_chunking ;
done