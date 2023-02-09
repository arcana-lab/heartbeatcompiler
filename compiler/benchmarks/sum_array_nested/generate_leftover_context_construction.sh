#!/bin/bash

handle_live_out=$1

echo "cxtsLeftover[index + LIVE_IN_ENV]   = ((uint64_t *)cxts)[index + LIVE_IN_ENV];" ;
if [ $handle_live_out == "true" ] ; then
  echo "cxtsLeftover[index + LIVE_OUT_ENV]  = ((uint64_t *)cxts)[index + LIVE_OUT_ENV];" ;
fi
