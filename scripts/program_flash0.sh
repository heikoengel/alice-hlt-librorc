#!/bin/bash

CMDFILE=/tmp/impact_$$.tmp

module load xilinx

echo 'setMode -bscan' > ${CMDFILE}
echo 'setCable -p auto' >> ${CMDFILE}
echo 'identify -inferir' >> ${CMDFILE}
echo 'identifyMPM' >> ${CMDFILE}
echo 'attachflash -position 1 -bpi "XCF128X"' >> ${CMDFILE}
echo 'assignfiletoattachedflash -position 1 -file "/opt/crorc-driver/firmware-files/crorc_fpgafw_latest.mcs"' >> ${CMDFILE}
echo 'Program -p 1 -dataWidth 16 -rs1 NONE -rs0 NONE -bpionly -e -loadfpga' >> ${CMDFILE}
echo "quit" >> ${CMDFILE}

impact -batch < ${CMDFILE}

rm ${CMDFILE}
