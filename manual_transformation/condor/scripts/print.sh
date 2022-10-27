#!/bin/bash

LIST="1 2 4 8 16 28 56"
cd $1

for num in $LIST ; do
	echo "### $num Core(s) ###"
	cat time$num.txt 2> /dev/null
	cat perf$num.txt 2> /dev/null
done
