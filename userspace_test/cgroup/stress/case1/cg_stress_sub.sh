#!/bin/bash

while [ 1 ]
do
  for i in `seq 0 200`
  do
    mkdir $1/$i
  done


  for i in `seq 0 200`
  do
    rmdir $1/$i
  done
  sleep 1
done

