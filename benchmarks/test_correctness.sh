#!/bin/bash

source ../enable ;

benchmarks=(floyd_warshall mandelbrot mandelbulb plus_reduce_array spmv srad)
impl=hbc
if [ ${1} ] ; then
  impl=${1}
fi

make clean &> /dev/null ;

for bench in ${benchmarks[@]} ; do
  echo "regression test: ${bench}" ;
  cd ${bench} ;
  make clean &> /dev/null ; make ${impl} INPUT_SIZE=testing TEST_CORRECTNESS=true &> /dev/null ;
  make run_${impl} ;
  cd ../ ;
done

make clean &> /dev/null ;
