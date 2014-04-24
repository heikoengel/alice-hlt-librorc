#!/bin/bash
#
# Start 12 PatterGenerator DMA Channels in background

DEV=$1
SCRIPTPATH=$(cd `dirname "${BASH_SOURCE[0]}"` && pwd)
LOGPATH=$SCRIPTPATH/log
BINPATH=$SCRIPTPATH/../build/release/tools_src

if [ -z $DEV ]; then
  echo "Please provide device number as argument - aborting."
  exit
fi

echo $SIZE

mkdir -p $LOGPATH

# allocate DMA buffer for all devices and channels
#$BINPATH/crorc_preallocator
#sleep 1

for CH in {0..3}
do
  PID=/var/run/hlt_in_${DEV}_${CH}.pid
  LOG=$LOGPATH/dev${DEV}_ch${CH}
  echo "Starting HLT_IN DMA from DIU on device ${DEV} Channel ${CH}"
  daemonize -o $LOG.log -e $LOG.err -p $PID -l $PID $BINPATH/dma_in --dev $DEV --ch $CH --source diu
  sleep 1
done
