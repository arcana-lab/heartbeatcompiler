#!/bin/bash

level=$1

for l in `seq 0 $(($level - 1))` ; do
  echo "itersArr[index++] = s$l;" ;
  echo "itersArr[index++] = m$l;" ;
done
