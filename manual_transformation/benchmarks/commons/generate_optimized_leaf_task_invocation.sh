#!/bin/bash

handle_live_out=$1

echo "taskparts::tpalrts_promote_via_nativefj([&] {" ;

if [ $handle_live_out == "true" ] ; then
  echo "  (*leafTask)(cxtFirst, 0, startIter + 1, med);" ;
else
  echo "  (*leafTask)(cxtFirst, startIter + 1, med);" ;
fi

echo "}, [&] {" ;

if [ $handle_live_out == "true" ] ; then
  echo "  (*leafTask)(cxtSecond, 1, med, maxIter);" ;
else
  echo "  (*leafTask)(cxtSecond, med, maxIter);" ;
fi

echo "}, [] { }, taskparts::bench_scheduler());" ;
