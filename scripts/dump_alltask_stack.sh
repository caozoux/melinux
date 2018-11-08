#!/bin/bash


ls /proc/ |
while read item
do
	if [ -f /proc/$item/stack ]
	then
		cat /proc/$item/comm
		cat /proc/$item/stack
	fi 
done

