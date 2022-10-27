#!/bin/bash

cd ../results/current_machine

for dir in */ ; do
	cd $dir

	for d in */ ; do
		if [ $1 = $d ] ; then
			rm -r $1
			break
		fi

		rm -rf $d/$1
	done

	cd ..
done
