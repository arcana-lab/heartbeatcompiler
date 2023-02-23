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
  ln -s ../commons/loop_handler.hpp loop_handler.hpp ;
  ln -s ../commons/loop_handler.cpp loop_handler.cpp ;
  ln -s ../commons/generate_code.sh generate_code.sh ;

  cd ../ ;

done
