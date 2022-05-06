#!/bin/bash

# get the number of cores on the currrent machine
NB_CORES=$( hwloc-ls --only core | wc -l )
#echo "nb cores = $NB_CORES"

# get the cpu frequency (in khz) of the current machine
read -a strarr <<< $( lscpu  | grep -i mhz )
export TASKPARTS_CPU_BASE_FREQUENCY_MHZ=$(printf '%.f' "${strarr[2]}")
export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=$(( $TASKPARTS_CPU_BASE_FREQUENCY_MHZ * 1000 ))
#echo $TASKPARTS_CPU_BASE_FREQUENCY_KHZ

export NB=300000000

OUTFILE=results.txt
EXECPATH="/home/rainey/heartbeatcompiler/compiler/example"

$EXECPATH/original_program $NB $NB >$OUTFILE
TASKPARTS_NUM_WORKERS=$NB_CORES $EXECPATH/program_with_heartbeat $NB $NB >>$OUTFILE
TASKPARTS_NUM_WORKERS=1 $EXECPATH/program_with_heartbeat $NB $NB >>$OUTFILE
