#!/bin/bash
#
# Reconfigure all GTX PLLs of a device
# Heiko Engel, 29.08.2013
#
#
# usage:
# ======
#
# get available devices and link rates
# ./change_link_rate.sh
#
# set a specific link rate:
#./change_link_rate.sh [DEVNUM] [PLLCFGNUM]


BASE=../build/release/tools_src
DEV=$1
PLLCFG=$2


####### Functions #######

# set/release QSFPs reset 
function set_qsfp_reset {
./$BASE/qsfpctrl -n $1 -q0 -r $2
./$BASE/qsfpctrl -n $1 -q1 -r $2
./$BASE/qsfpctrl -n $1 -q2 -r $2
}

function get_refclk {
REFCLK=`./$BASE/gtxctrl -P | sed -n 's/\['$PLLCFG'\] RefClk=\([0-9\.]\+\) MHz.*/\1/p'`
}



####### Main #######

# check if parameter 1 is provided/non-empty
if [ -z $1 ]; then
  echo "No device given - aborting."
  echo "Usage: ./change_link_rate.sh [DEVNUM] [PLLCFGNUM]"
  echo ""
  ./$BASE/rorctl -l
  echo ""
  ./$BASE/gtxctrl -P
  exit
fi

# check if parameter 2 is provided/non-empty
if [ -z $PLLCFG ]; then
  echo "No PLL config given - aborting."
  echo "Usage: ./change_link_rate.sh [DEVNUM] [PLLCFGNUM]"
  echo ""
  ./$BASE/gtxctrl -P
  exit
fi

# get reference clock value from selected PLL settings
get_refclk
if [ -z $REFCLK ]; then
  echo "Invalid PLL config selected: $PLLCFG, aborting."
  exit
fi


# reset all QSFPs
echo "Resetting all QSFP"
set_qsfp_reset $DEV 1

# reset all GTX
echo "Resetting all GTX"
./$BASE/gtxctrl -n $DEV -r1

# set according reference clock frequency
echo "Setting Reference Clock"
./$BASE/refclkgenctrl -n $DEV -w $REFCLK

# reconfigure all GTX via DRP
echo "Reconfiguring GTX via DRP"
./$BASE/gtxctrl -n $DEV -p $PLLCFG

# release all GTX resets
echo "Releasing GTX resets"
./$BASE/gtxctrl -n $DEV -r0

# release QSFP resets on dev0 and dev1
echo "Releasing QSFP resets"
set_qsfp_reset $DEV 0 

# give link a chance to lock again
sleep 1

# clear error counter
echo "Resetting Link Error Counters"
./$BASE/gtxctrl -n $DEV -x
