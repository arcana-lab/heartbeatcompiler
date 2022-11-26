#!/bin/bash

# make clean ; make correctness ;

for i in `seq 1 5` ; do

  TASKPARTS_NUM_WORKERS=1 taskset -c 0 make run_correctness ;

done
