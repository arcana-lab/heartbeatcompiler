To submit:
```
condor_submit Mike.con
```

To check the status of your job:
```
condor_q
```

To kill all of your jobs:
```
condor_rm `whoami`
```

To kill a single job:
```
condor_rm JOB_ID
```
where JOB_ID is printed when you submit a job
