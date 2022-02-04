To submit:
```
condor_submit Mike.con
```

To check the status of your job:
```
condor_q
```

To check on details of your job:
```
condor_q -analyze JOB_ID
```
where JOB_ID is printed when you submit a job

To kill all of your jobs:
```
condor_rm `whoami`
```

To kill a single job:
```
condor_rm JOB_ID
```
where JOB_ID is printed when you submit a job

To enable burnP6:
```
source /project/extra/burnCPU/enable
```

To run it on core 5:
```
taskset -c 5 burnP6 & 
```

To kill all burnP6:
```
killall burnP6 
```

To disable TurboBoost on any Intel-based machine, you need to have at least half physical cores of a socket busy.
