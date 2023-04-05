#!/bin/bash
HEARTBEATCOMPILER_REPO=$1

pushd . ;

cd ${HEARTBEATCOMPILER_REPO}/evaluation ;

# run the evaluation starting here
echo "running scaling_baseline script" ;
./scaling_baseline.sh ;

echo "running scaling_competitor script" ;
./scaling_competitor.sh ;

echo "running scaling_heartbeat script" ;
./scaling_heartbeat.sh ;

popd ;
