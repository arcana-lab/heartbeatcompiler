#!/bin/bash

# source the default environment
source /home/yso0488/.bash_profile ;

# go to the executing folder
cd /home/yso0488/projects/heartbeatcompiler/manual_transformation/condor ;

# make the progress file
PROGRESS="progress.txt" ;
rm $PROGRESS ;
touch $PROGRESS ;

# pipeline starts
echo "Pipeline starts" >> $PROGRESS ;


echo "Hello from pipeline" ;


# pipeline finishes
echo "Pipeline finishes" >> $PROGRESS ;

exit 0 ;
