#!/bin/bash
if [ $# != 3 ] &&  [ $# != 2 ]
then
	echo "   $0 -r/w/p \$address \$val"
	exit
fi

if [ $1 == "-r" ]
then
	echo $2 > /sys/devices/virtual/misc/mem_dir/mem_tag
	cat /sys/devices/virtual/misc/mem_dir/mem_op
elif [ $1 == '-w' ]
then
	echo "$2 > /sys/devices/virtual/misc/mem_dir/mem_tag"
	echo "$3 > /sys/devices/virtual/misc/mem_dir/mem_op"
elif [ $1 == '-p' ]
then
	echo $2 > /sys/devices/virtual/misc/mem_dir/mem_phy
	cat /sys/devices/virtual/misc/mem_dir/mem_phy
elif [ $1 == '-v' ]
then
	echo $2 > /sys/devices/virtual/misc/mem_dir/mem_vir
	cat /sys/devices/virtual/misc/mem_dir/mem_vir
else
	:
fi
#/sys/devices/virtual/misc/mem_dir/mem_op
#/sys/devices/virtual/misc/mem_dir/mem_tag
