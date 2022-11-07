#!/bin/bash

cd /home/yso0488/projects/heartbeatcompiler/manual_transformation/utility ;

source /project/extra/llvm/14.0.6/enable ;

clang tsc.c -o tsc ;

./tsc ;
