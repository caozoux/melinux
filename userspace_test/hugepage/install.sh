#!/bin/bash

if [ ! -d "/mnt/huge" ]
then
	#sudo echo 200 > /proc/sys/vm/nr_hugepages
	sudo mount -t hugetlbfs none /mnt
	sudo mkdir /mnt/huge

fi

sudo ./test
