#!/bin/bash

level=$1
handle_live_out=$2

for l in `seq 0 $(($level - 1))` ; do
  echo "extern" ;
  if [ $handle_live_out == "true" ] ; then
    echo "int64_t HEARTBEAT_loop${l}_slice(uint64_t *, uint64_t, uint64_t, uint64_t" ;
  else
    echo "int64_t HEARTBEAT_loop${l}_slice(uint64_t *, uint64_t, uint64_t" ;
  fi
 
  for i in `seq 0 $(($l - 1))` ; do
    echo ", uint64_t, uint64_t" ;
  done

  echo ");" ;
done
