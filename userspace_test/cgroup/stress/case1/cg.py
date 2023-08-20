#!/bin/python3
import os
import re
import datetime
import time

listdir=os.listdir("/proc/")
diskkey={}
totaltime=0
for item in listdir:
    ptask_dir="/proc/"+item
    try:
        start_dt= time.time()
        res=open(ptask_dir+"/cgroup",'r').read()
        end_dt = time.time()
        delta = float(end_dt - start_dt) * 1000.0
        totaltime += float(end_dt - start_dt) * 1000.0
        if delta > 10:
          print("time cost:", float(end_dt - start_dt) * 1000.0, "ms")
    except Exception as e:
        continue

print(totaltime)
