#!/bin/bash

level=$1

echo "rc = loop_handler(cxts, receivingLevel, leftoverTasks, leafTasks," ;

echo "s0, m0"
for l in `seq 1 $(($level - 1))` ; do
  echo ", s$l, m$l" ;
done

echo ");";
