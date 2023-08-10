#!/bin/bash

# enter the experiment here
cd results/overhead ;

for bench in `ls` ; do
	rm -rf ${bench}/sp_aca ;
done

cd ../../ ;
