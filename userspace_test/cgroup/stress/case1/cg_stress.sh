#!/bin/bash

curdir=`pwd`
mkdir /sys/fs/cgroup/memory/test
for i in `seq 0 25`
do
  mkdir /sys/fs/cgroup/memory/test/test$i/
  ${curdir}/cg_stress_sub.sh /sys/fs/cgroup/memory/test/test$i &
done

