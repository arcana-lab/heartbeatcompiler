#!/bin/bash

cd /nfs-scratch/bhp1038/heart/manual_transformation/condor
FILE=progress.log
OPENMP=OMP_NUM_THREADS
HEARTB=TASKPARTS_NUM_WORKERS
rm -f ${FILE}

echo "### Pipeline Starts ###" > ${FILE}

# Setup environment for the heartbeat evaluation pipeline
echo "### Setup Stage ###" >> ${FILE}
source ./bin/setup >> ${FILE}

# Compile and collect metrics for the baseline
echo "### Baseline Stage ###" >> ${FILE}
all "baseline" >> ${FILE}

# OpenCilk
echo "### OpenCilk Stage ###" >> ${FILE}
all "opencilk" "CILK_NWORKERS" >> ${FILE}

# OpenMP Static Scheduler
echo "### OpenMP Static Stage ###" >> ${FILE}
all "openmp" $OPENMP >> ${FILE}

# OpenMP Dynamic Scheduler
# echo "### OpenMP Dynamic Stage ###" >> ${FILE}
# all "openmp_dynamic" $OPENMP >> ${FILE}

# OpenMP Guided Scheduler
echo "### OpenMP Guided Stage ###" >> ${FILE}
all "openmp_guided" $OPENMP >> ${FILE}

# Compile and collect metrics for Heartbeat
# echo "### Heartbeat Stage ###" >> ${FILE}
# ./bin/run_heartbeat >> ${FILE}

echo "### Pipeline Ends ###" >> ${FILE}
exit 0
