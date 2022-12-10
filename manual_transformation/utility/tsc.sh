#!/bin/bash

cd /nfs-scratch/yso0488/heartbeatcompiler0/manual_transformation/utility ;

source /project/extra/llvm/14.0.6/enable ;
source /project/extra/burnCPU/enable ;

killall burnP6 &> /dev/null ;

for coreID in `seq -s' ' 0 2 6` ; do
  taskset -c ${coreID} burnP6 &
done

clang tsc.c -o tsc ;

taskset -c 8 ./tsc ;

killall burnP6 &> /dev/null ;
