#!/bin/bash


MODULE_FILE=`ls *.ko | sed -n "1p"`

MODULE_INSMOD=`lsmod | grep $MODULE_FILE`

if [ "$?" == "0" ]
then
	echo "11"
	sudo insmod $MODULE_FILE
else
	echo "22"
	sudo rmmod $MODULE_FILE && sudo insmod $MODULE_FILE
fi

