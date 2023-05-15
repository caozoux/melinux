#!/bin/bash


if [ -d /sys/fs/cgroup/cpu,cpuacct/test ]
then
	cgdelete -g cpuacct:test
fi

killall cpubusy
mkdir /sys/fs/cgroup/cpu,cpuacct/test
#echo 1000  > /sys/fs/cgroup/cpu,cpuacct/test/cpu.cfs_period_us
echo 100000  > /sys/fs/cgroup/cpu,cpuacct/test/cpu.cfs_quota_us
cgexec  -g cpuacct:test ./OUT/cpubusy -t busy -c 3 &

