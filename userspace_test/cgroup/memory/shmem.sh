#!/bin/bash

source ../bashe.sh

TESTDIR="/sys/fs/cgroup/memory/test"
if [ ! -d ${TESTDIR} ]
then
	sudo mkdir ${TESTDIR}
fi

echo $$ > ${TESTDIR}
echo 104857600  >> /sys/fs/cgroup/memory/test/memory.max_usage_in_bytes

write_cache /tmp/testdata  1001024


write_shmem testdata  1001024

