#!/bin/bash

# arg1: filename
# arg2: write size kB
function  write_cache() {
	dd if=/dev/zero of=$1 bs=1K count=$2	
}


function  write_shmem() {
	dd if=/dev/zero of=/dev/shm/$1 bs=1K count=$2	
}

