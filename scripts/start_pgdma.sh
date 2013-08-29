#!/bin/bash
#
# Start 12 PatterGenerator DMA Channels in background

DEV=$1
SIZE=0x1000
SCRIPTPATH=$(cd `dirname "${BASH_SOURCE[0]}"` && pwd)
LOGPATH=$SCRIPTPATH/log
BINPATH=$SCRIPTPATH/../build/release/tools_src

if [ -z $DEV ]; then
  echo "Please provide device number as argument - aborting."
  exit
fi

mkdir -p $LOGPATH

for CH in {0..11}
do
  PID=/var/run/pgdma_${DEV}_${CH}.pid
  LOG=$LOGPATH/dev${DEV}_ch${CH}
  echo "Starting PatterGenerator DMA on device ${DEV} Channel ${CH}"
  daemonize -o $LOG.log -e $LOG.err -p $PID -l $PID $BINPATH/pgdma_continuous --dev $DEV --ch $CH --size $SIZE
done
