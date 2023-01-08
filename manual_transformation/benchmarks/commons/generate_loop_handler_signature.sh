#!/bin/bash

level=$1
handle_live_out=$2

if [ $handle_live_out == "true" ] ; then
  echo "int64_t (*leftoverTasks[])(uint64_t *, uint64_t, uint64_t *)," ;
  echo "int64_t (*leafTasks[])(uint64_t *, uint64_t, uint64_t, uint64_t)," ;
else
  echo "int64_t (*leftoverTasks[])(uint64_t *, uint64_t *)," ;
  echo "int64_t (*leafTasks[])(uint64_t *, uint64_t, uint64_t)," ;
fi

echo "uint64_t s0, uint64_t m0" ;
for l in `seq 1 $(($level - 1))` ; do
  echo ", uint64_t s$l, uint64_t m$l" ;
done
