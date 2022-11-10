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
  
  ln -s ../commons/Makefile Makefile ;
  ln -s ../commons/test_correctness.sh test_correctness.sh ;

  cd ../ ;

done

exit 0 ;
