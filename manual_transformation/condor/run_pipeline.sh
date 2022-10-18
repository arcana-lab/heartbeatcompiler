#!/bin/bash

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
	
	BINARY=$ROOT/results/current_machine/binary/$d/baseline
	mkdir -p $BINARY && cp $d/baseline "$_"
	
	(
		TIME=$ROOT/results/time/$d/baseline
		PERF=$ROOT/results/LLC-load-misses/$d/baseline
		
		rm $TIME/time.txt
		rm $PERF/perf.txt
		cd $BINARY

		for n in {1..5} ; do
			echo "ITERATION $n" >> $TXT

			mkdir -p $TIME
			echo "-----$n-----" >> $TIME/time.txt
			{ time -p taskset -c 0 ./baseline >> $TXT; } 2>> $TIME/time.txt

			mkdir -p $PERF
			echo "-----$n-----" >> $PERF/perf.txt
			{ perf stat -e LLC-load-misses ./baseline >> $TXT; } 2>> $PERF/perf.txt
		done
	)
done

echo "-----EXIT-----" >> $TXT
exit 0
