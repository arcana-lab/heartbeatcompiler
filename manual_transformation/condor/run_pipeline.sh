#!/bin/bash

source /project/extra/llvm/9.0.0/enable
export TASKPARTS_CPU_BASE_FREQUENCY_KHZ=240000

ROOT=/nfs-scratch/bhp1038/heart/manual_transformation
TXT=$ROOT/condor/progress.txt

cd $ROOT/benchmarks
make > $TXT 2>&1

for d in */ ; do
	if [ $d = "scripts/" ] ; then
		continue
	fi

	echo "-----COMPILATION-----" >> $TXT
	make -C $d >> $TXT
	
	FOLDER=$ROOT/results/current_machine
	BINARY=$FOLDER/binary/$d/baseline
	mkdir -p $BINARY && cp $d/baseline "$_"
	
	(
		TIME=$FOLDER/time/$d/baseline
		PERF=$FOLDER/LLC-load-misses/$d/baseline
		
		mkdir -p $TIME && rm -f $TIME/time.txt
		mkdir -p $PERF && rm -f $PERF/perf.txt
		cd $BINARY

		for n in {1..5} ; do
			echo "ITERATION $n" >> $TXT

			{
				time -p taskset -c 0 ./baseline >> $TXT;
			} 2>&1 | grep -Po "real \K\d+\.\d+" >> $TIME/time.txt

			{
				perf stat -e LLC-load-misses ./baseline >> $TXT;
			} 2>> $PERF/perf.txt
		done
	)
done

echo "-----EXIT-----" >> $TXT
exit 0
