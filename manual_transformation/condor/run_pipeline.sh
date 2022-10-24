#!/bin/bash

PROGRESS_FILE="progress.log"
rm -f ${PROGRESS_FILE}

echo "### Pipeline Starts ###" > ${PROGRESS_FILE}

# Setup environment for the heartbeat evaluation pipeline
echo "### Setup Stage ###" >> ${PROGRESS_FILE}
source ./bin/setup >> ${PROGRESS_FILE}

# Compile and collect metrics for the baseline
echo "### Baseline Stage ###" >> ${PROGRESS_FILE}
all "baseline" >> ${PROGRESS_FILE}

# # Compile and collect metrics for the OpenCilk
echo "### OpenCilk Stage ###" >> ${PROGRESS_FILE}
all "opencilk" >> ${PROGRESS_FILE}

# # Compile and collect metrics for the OpenMP
echo "### OpenMP Stage ###" >> ${PROGRESS_FILE}
all "openmp" >> ${PROGRESS_FILE}

# # Compile and collect metrics for the Heartbeat
# echo "### Heartbeat Stage ###" >> ${PROGRESS_FILE}
# ./bin/run_heartbeat >> ${PROGRESS_FILE}

echo "### Pipeline Ends ###" >> ${PROGRESS_FILE}

exit 0
