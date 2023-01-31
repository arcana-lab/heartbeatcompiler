#!/bin/bash

level=$1
handle_live_out=$2

if [ $handle_live_out == "true" ] ; then
  echo "void (*leftoverTasks[])(void *, uint64_t, void *)," ;
else
  echo "void (*leftoverTasks[])(void *, void *)," ;
fi

echo "uint64_t s0, uint64_t m0" ;
for l in `seq 1 $(($level - 1))` ; do
  echo ", uint64_t s$l, uint64_t m$l" ;
done
