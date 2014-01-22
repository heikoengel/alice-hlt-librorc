#!/bin/bash

#EVENT_SIZES="96 160 288 544 1056 2080 4128 8224 16416 32800 65568"
EVENT_SIZES="64 128 256 512 1024 2048 4096 8192 16384 32768 65536"


for EVENT_SIZE in $EVENT_SIZES
do
    EVENT_SIZE_REAL=$(($EVENT_SIZE + 32))
    EVENT_SIZE_HEX="0x`echo "obase=16; $EVENT_SIZE" | bc`"
    echo "$EVENT_SIZE = $EVENT_SIZE_HEX -> $EVENT_SIZE_REAL"

    ./start_hlt_out.sh 0 $EVENT_SIZE_HEX
    ../build/release/tools_src/dma_monitor --iterations 31 | grep Combined | awk '{print '${EVENT_SIZE_REAL}' " ; " $8 }' >> hlt_out.csv
    ./stop_hlt_out.sh 0
    sleep 4
done
