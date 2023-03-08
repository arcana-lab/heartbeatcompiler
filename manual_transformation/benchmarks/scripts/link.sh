#!/bin/bash

for bench in `ls` ; do
  if ! test -d $bench ; then
    continue ;
  fi

  if [ $bench == "scripts" ] ; then
    continue ;
  fi

  if [ $bench == "commons" ] ; then
    continue ;
  fi

  cd $bench ;

  ln -s ../../../benchmarks/$bench/* . ;

  ln -s ../commons/Makefile Makefile_heartbeat ;

  cd ../ ;

done
