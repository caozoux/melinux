#!/bin/bash 
total_lost=0
total_send=0
iperfargs="-c 192.168.122.81 -u -b 50M -P 5 -t 5"
rm -f log
iperf $iperfargs  | grep "(0" > log
while read line
do
	result=`echo $line |  awk '{print $(NF-1)}'`
	lost_package=`echo $result |  awk -F/ '{print $1}'`
	send_package=`echo $result |  awk -F/ '{print $2}'`
	total_lost=`expr $total_lost + $lost_package`
	total_send=`expr $total_send + $send_package`
	echo $line lost:$lost_package  send:$send_package
done < log

echo lost_upd: $total_lost
echo total_udp: $total_send
rm -f log
