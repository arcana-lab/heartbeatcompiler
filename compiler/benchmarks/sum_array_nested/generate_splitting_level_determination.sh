#!/bin/bash

level=$1

echo "if ((m0 - s0) >= (SMALLEST_GRANULARITY + 1)) {" ;
echo "  splittingLevel = 0;" ;

for l in `seq 1 $(($level - 1))` ; do
  echo "} else if ((m$l - s$l) >= (SMALLEST_GRANULARITY + 1)) {" ;
  echo "  splittingLevel = $l;" ;
done

echo "}"
