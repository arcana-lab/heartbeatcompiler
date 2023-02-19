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
  
  if test -e Makefile ; then
    make clean ; 
  fi

  cd ../ ;

done
