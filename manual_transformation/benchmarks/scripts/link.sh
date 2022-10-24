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
  ln -s ../scripts/test_correctness.sh test_correctness.sh ;

  cd ../ ;

done

exit 0 ;
