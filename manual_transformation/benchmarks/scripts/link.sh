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

  if [ $bench == "results" ] ; then
    continue ;
  fi

  if [ $bench == "plots" ] ; then
    continue ;
  fi

  cd $bench ;
  
  ln -s ../commons/Makefile Makefile ;
  ln -s ../commons/loop_handler.hpp loop_handler.hpp ;
  ln -s ../commons/rollforward_handler.hpp rollforward_handler.hpp ;
  ln -s ../commons/generate_code.sh generate_code.sh ;

  cd ../ ;

done

exit 0 ;
