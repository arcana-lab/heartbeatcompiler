#!/bin/bash

level=$1
handle_live_out=$2

echo "switch (splittingLevel) {" ;

for l in `seq 0 $(($level - 1))` ; do
  echo "  case $l:" ;
  echo "    taskparts::tpalrts_promote_via_nativefj([&] {" ;
  if [ $handle_live_out == "true" ] ; then
    echo "      (*leftoverTasks[leftoverTaskIndex])((void *)cxtsLeftover, 0, (void *)itersArr);"
  else
    echo "      (*leftoverTasks[leftoverTaskIndex])((void *)cxtsLeftover, (void *)itersArr);"
  fi
  echo "    }, [&] {" ;
  echo "      taskparts::tpalrts_promote_via_nativefj([&] {" ;
  if [ $handle_live_out == "true" ] ; then
    echo "        HEARTBEAT_loop${l}_slice((void *)cxtsFirst, 0," ;
  else
    echo "        HEARTBEAT_loop${l}_slice((void *)cxtsFirst," ;
  fi
  
  for i in `seq 0 $(($l - 1))` ; do
    echo "          0, 0, " ;
  done
  echo "          s$l + 1, med" ;
  
  echo "        );" ;
  echo "      }, [&] {" ;
  if [ $handle_live_out == "true" ] ; then
    echo "        HEARTBEAT_loop${l}_slice((void *)cxtsSecond, 1," ;
  else
    echo "        HEARTBEAT_loop${l}_slice((void *)cxtsSecond," ;
  fi

  for i in `seq 0 $(($l - 1))` ; do
    echo "          0, 0, " ;
  done
  echo "          med, m$l" ;

  echo "        );" ;
  echo "      }, [&] { }, taskparts::bench_scheduler());" ;
  echo "    }, [&] { }, taskparts::bench_scheduler());" ;
  echo "    break;" ;
done

echo "  default:" ;
echo "    exit(1);" ;

echo "}" ;
