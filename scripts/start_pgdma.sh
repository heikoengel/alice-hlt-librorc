#!/bin/bash
#
# Start 12 PatterGenerator DMA Channels in background

DEV=$1
SIZE=$2
SCRIPTPATH=$(cd `dirname "${BASH_SOURCE[0]}"` && pwd)
LOGPATH=$SCRIPTPATH/log
BINPATH=$LIBRORC_BUILD/tools_src

if [ -z $DEV ]; then
  echo "Please provide device number as argument - aborting."
  exit
fi

if [ -z $2 ]; then
  SIZE=0x1000
fi

echo $SIZE

mkdir -p $LOGPATH

# allocate DMA buffer for all devices and channels
#$BINPATH/crorc_preallocator
#sleep 1

for CH in {0..11}
do
  PID=${LOGPATH}/pgdma_$(hostname)_${DEV}_${CH}.pid
  LOG=$LOGPATH/pgdma_$(hostname)_${DEV}_${CH}
  echo "Starting PatterGenerator DMA on device ${DEV} Channel ${CH}"
  daemonize -o $LOG.log -e $LOG.err -p $PID -l $PID $BINPATH/dma_in --dev $DEV --ch $CH --size $SIZE --source pg
  sleep 1
done
