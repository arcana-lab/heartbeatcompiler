#!/bin/bash

bench=floyd_warshall
cd /nfs-scratch/yso0488/heartbeatcompiler0/benchmarks/${bench}/condor ;

./time_floyd_warshall.sh tpal 1K
./time_floyd_warshall.sh tpal 2K
