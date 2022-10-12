#!/bin/bash

for tst in `ls` ; do
  if ! test -d $tst ; then
    continue ;
  fi

  if [ $tst == "scripts" ] ; then
    continue ;
  fi

  cd $tst ;
  
  if test -e Makefile ; then
    make clean ; 
  fi

  cd ../ ;

done

exit 0 ;
