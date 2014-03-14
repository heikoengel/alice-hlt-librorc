#!/bin/bash

PROG="../build/release/tools_src/bufferstats"
FILE=sg_entries.csv

../build/release/tools_src/crorc_preallocator >> /dev/null

for I in {0..23}
do
    echo -n `$PROG -n 0 -b $I | grep "Size:" | awk '{print $4}' | cut -d"(" -f2` >> $FILE
    echo -n " ; "                                                                >> $FILE
    $PROG -n 0 -b $I | grep "SG Entries" | awk '{print $5}'                      >> $FILE
done
