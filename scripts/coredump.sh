#!/bin/bash


ulimit -c unlimited
echo "/tmp/coredump/core.%e,%p" > /proc/sys/kernel/core_pattern
