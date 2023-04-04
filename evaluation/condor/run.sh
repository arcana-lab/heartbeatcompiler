#!/bin/bash
HEARTBEATCOMPILER_REPO=$1

pushd . ;

cd ${HEARTBEATCOMPILER_REPO}/evaluation ;

# run the evaluation starting here
echo `pwd` ;

popd ;
