#!/bin/bash

CMDFILE=/tmp/impact_$$.tmp

#module load xil/141

# function to program single FPGA
program_fpga()
{
  PROGCMDS=/tmp/impact_prog_$$.tmp
  FILE=$1
  PORT=$2
  DEVICE=1
    echo "Programming device at port $PORT"
    echo "setMode -bscan" > ${PROGCMDS}
    echo "setCable -p ${PORT}" >> ${PROGCMDS}
    echo "identify" >> ${PROGCMDS}
    echo "assignfile -p ${DEVICE} -file ${FILE}" >> ${PROGCMDS}
    echo "program -p $DEVICE" >> ${PROGCMDS}
    echo "quit" >> ${PROGCMDS}
    RESULT=`impact -batch < ${PROGCMDS} 2>&1 | grep ERROR`  
    if [ -n "${RESULT}" ]; then
      echo "Programming ${PORT} failed!"
      echo $RESULT
    fi
    rm ${PROGCMDS}
}

if [ -z "${LIBRORC_FIRMWARE_PATH}" ]; then
  echo "ERROR: Environment variable LIBRORC_FIRMWARE_PATH is not set!"
  exit
fi

FILE="${LIBRORC_FIRMWARE_PATH}/crorc_fpgafw_latest.bit"

if [ ! -f ${FILE} ]; then
  echo "Bitfile not found: ${FILE}"
  exit
fi

#get list of programmers
echo "setMode -bscan" > ${CMDFILE}
echo "listusbcables" >> ${CMDFILE}
echo "quit" >> ${CMDFILE}

IMPOUT=`impact -batch < ${CMDFILE} 2>&1 | sed -n 's/^port=\(.*\),.*$/\1/gp'`

if [ -z "$IMPOUT" ]; then
  echo "No programming cables found!"
fi

for PORT in ${IMPOUT[@]}
do
  echo "Found port ${PORT}"
  program_fpga ${FILE} ${PORT}

done

rm ${CMDFILE}



