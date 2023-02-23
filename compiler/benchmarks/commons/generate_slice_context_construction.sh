#!/bin/bash

handle_live_out=$1

echo "cxtsFirst[splittingLevel * CACHELINE + LIVE_IN_ENV]   = ((uint64_t *)cxts)[splittingLevel * CACHELINE + LIVE_IN_ENV];" ;
echo "cxtsSecond[splittingLevel * CACHELINE + LIVE_IN_ENV]  = ((uint64_t *)cxts)[splittingLevel * CACHELINE + LIVE_IN_ENV];" ;
if [ $handle_live_out == "true" ] ; then
  echo "cxtsFirst[splittingLevel * CACHELINE + LIVE_OUT_ENV]  = ((uint64_t *)cxts)[splittingLevel * CACHELINE + LIVE_OUT_ENV];" ;
  echo "cxtsSecond[splittingLevel * CACHELINE + LIVE_OUT_ENV] = ((uint64_t *)cxts)[splittingLevel * CACHELINE + LIVE_OUT_ENV];" ;
fi
