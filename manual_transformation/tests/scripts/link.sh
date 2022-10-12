#!/bin/bash

for tst in `ls` ; do
  if ! test -d $tst ; then
    continue ;
  fi

  if [ $tst == "scripts" ] ; then
    continue ;
  fi

  cd $tst ;
  
  ln -s ../scripts/Makefile Makefile ;

  cd ../ ;

done
