#!/bin/bash

cd /nfs-scratch/bhp1038/heart/manual_transformation/condor
PROGRESS_FILE="progress.log"
rm -f ${PROGRESS_FILE}

echo "### Pipeline Starts ###" > ${PROGRESS_FILE}

# Setup environment for the heartbeat evaluation pipeline
echo "### Setup Stage ###" >> ${PROGRESS_FILE}
source ./bin/setup >> ${PROGRESS_FILE}

# Compile and collect metrics for the baseline
echo "### Baseline Stage ###" >> ${PROGRESS_FILE}
all "baseline" >> ${PROGRESS_FILE}

# OpenCilk
echo "### OpenCilk Stage ###" >> ${PROGRESS_FILE}
all "opencilk" >> ${PROGRESS_FILE}

# OpenMP Static Scheduler
echo "### OpenMP Static Stage ###" >> ${PROGRESS_FILE}
all "openmp" >> ${PROGRESS_FILE}

# OpenMP Dynamic Scheduler
echo "### OpenMP Dynamic Stage ###" >> ${PROGRESS_FILE}
all "openmp_dynamic" >> ${PROGRESS_FILE}

# OpenMP Guided Scheduler
echo "### OpenMP Guided Stage ###" >> ${PROGRESS_FILE}
all "openmp_guided" >> ${PROGRESS_FILE}

# Compile and collect metrics for Heartbeat
# echo "### Heartbeat Stage ###" >> ${PROGRESS_FILE}
# ./bin/run_heartbeat >> ${PROGRESS_FILE}

echo "### Pipeline Ends ###" >> ${PROGRESS_FILE}
exit 0
