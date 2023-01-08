#!/bin/bash

handle_live_out=$1

echo "cxtFirst[LIVE_IN_ENV]   = cxt[LIVE_IN_ENV];" ;
echo "cxtSecond[LIVE_IN_ENV]  = cxt[LIVE_IN_ENV];" ;
if [ $handle_live_out == "true" ] ; then
  echo "cxtSecond[LIVE_OUT_ENV] = cxt[LIVE_OUT_ENV];" ;
  echo "cxtFirst[LIVE_OUT_ENV]  = cxt[LIVE_OUT_ENV];" ;
fi
