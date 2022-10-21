#!/bin/bash

# Change this to your path or pass in as an environment variable
ROOT_FOLDER="/home/yso0488/projects/heartbeatcompiler/manual_transformation/condor"
cd ${ROOT_FOLDER} ;

PROGRESS_FILE="progress.log"
rm ${PROGRESS_FILE} ;
touch ${PROGRESS_FILE} ;

echo "### Pipeline Starts ###" >> ${PROGRESS_FILE} 2>&1 ;

# Setup environment for the heartbeat evaluation pipeline
echo "### Setup Stage ###" >> ${PROGRESS_FILE} 2>&1 ;
source ./bin/setup >> ${PROGRESS_FILE} 2>&1 ;

# Compile and collect metrics for the baseline
echo "### Baseline Stage ###" >> ${PROGRESS_FILE} 2>&1 ;
./bin/run_baseline >> ${PROGRESS_FILE} 2>&1 ;

# # Compile and collect metrics for the OpenCilk
# echo "### OpenCilk Stage ###" >> ${PROGRESS_FILE} 2>&1 ;
# ./bin/run_opencilk >> ${PROGRESS_FILE} 2>&1 ;

# # Compile and collect metrics for the OpenMP
# echo "### OpenMP Stage ###" >> ${PROGRESS_FILE} 2>&1 ;
# ./bin/run_openmp >> ${PROGRESS_FILE} 2>&1 ;

# # Compile and collect metrics for the Heartbeat
# echo "### Heartbeat Stage ###" >> ${PROGRESS_FILE} 2>&1 ;
# ./bin/run_heartbeat >> ${PROGRESS_FILE} 2>&1 ;

echo "### Pipeline Ends ###" >> ${PROGRESS_FILE} 2>&1 ;

exit 0 ;
