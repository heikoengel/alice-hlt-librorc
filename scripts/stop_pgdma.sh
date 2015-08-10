#!/bin/bash

DEV=$1
SCRIPTPATH=$(cd `dirname "${BASH_SOURCE[0]}"` && pwd)
LOGPATH=$SCRIPTPATH/log
BINPATH=$LIBRORC_BUILD/tools_src

if [ -z $DEV ]; then
  echo "Please provide device number as argument - aborting."
  exit
fi


for CH in {0..11}
do
  PID=$LOGPATH/pgdma_$(hostname)_${DEV}_${CH}.pid
  if [ -f $PID ]; then
    kill -s 2 `cat $PID`
  else
    echo "No PID file found for device ${DEV} channel ${CH}"
  fi
done
