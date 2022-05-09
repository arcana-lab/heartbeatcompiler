#!/bin/bash

for i in `ls` ; do
  if ! test -d $i ; then
    continue ;
  fi
  pushd ./ ;
  cd $i ;
  if test -e Makefile ; then
    make clean ; 
  fi
  popd ;
done
