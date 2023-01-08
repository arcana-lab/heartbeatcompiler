#!/bin/bash

handle_live_out=$1

echo "taskparts::tpalrts_promote_via_nativefj([&] {" ;

if [ $handle_live_out == "true" ] ; then
  echo "  (*leafTasks[leafTaskIndex])(&cxtsFirst[receivingLevel * CACHELINE], 0, itersArr[receivingLevel * 2 + START_ITER] + 1, med);" ;
else
  echo "  (*leafTasks[leafTaskIndex])(&cxtsFirst[receivingLevel * CACHELINE], itersArr[receivingLevel * 2 + START_ITER] + 1, med);" ;
fi

echo "}, [&] {" ;

if [ $handle_live_out == "true" ] ; then
  echo "  (*leafTasks[leafTaskIndex])(&cxtsSecond[receivingLevel * CACHELINE], 1, med, itersArr[receivingLevel * 2 + MAX_ITER]);" ;
else
  echo "  (*leafTasks[leafTaskIndex])(&cxtsSecond[receivingLevel * CACHELINE], med, itersArr[receivingLevel * 2 + MAX_ITER]);" ;
fi

echo "}, [] { }, taskparts::bench_scheduler());" ;
