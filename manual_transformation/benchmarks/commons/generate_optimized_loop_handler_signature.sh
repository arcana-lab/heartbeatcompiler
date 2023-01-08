#!/bin/bash

handle_live_out=$1

if [ $handle_live_out == "true" ] ; then
  echo "int64_t (*leafTask)(uint64_t *, uint64_t, uint64_t, uint64_t)" ;
else
  echo "int64_t (*leafTask)(uint64_t *, uint64_t, uint64_t)" ;
fi
