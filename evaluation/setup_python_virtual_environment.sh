#!/bin/bash

# Get path to this file
THIS_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )" ;
repoPath=`git rev-parse --show-toplevel`

# Setup the python virtual environment for noelleGym
virtualEnvDir="${repoPath}/evaluation/pythonVirtualEnv" ;
if ! test -d ${virtualEnvDir} ; then
    mkdir ${virtualEnvDir} ;

    virtualenv -p python3 ${virtualEnvDir} ;

    source ${virtualEnvDir}/bin/activate ;

    pip install --upgrade pip ;

    pip install matplotlib ;
    pip install numpy ;
    pip install pandas ;
    pip install plotly==5.15.0 ;
    pip install -U kaleido ;

else
  source ${virtualEnvDir}/bin/activate ;
fi
