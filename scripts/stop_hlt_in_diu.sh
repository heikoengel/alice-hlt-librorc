#!/bin/bash

DEV=$1
if [ -z $DEV ]; then
  echo "Please provide device number as argument - aborting."
  exit
fi


for CH in {0..3}
do
  PID=/var/run/hlt_in_${DEV}_${CH}.pid
  if [ -f $PID ]; then
    kill -s 2 `cat $PID`
  else
    echo "No PID file found for device ${DEV} channel ${CH}"
  fi
done
