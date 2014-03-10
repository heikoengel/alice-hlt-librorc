#!/bin/bash

PROG="../build/release/tools_src/bufferstats"

#../build/release/tools_src/crorc_preallocator

for I in {0..23}
do
    echo -n `$PROG -n 0 -b 0 | grep "Size:" | awk '{print $4}' | cut -d"(" -f2`
    echo -n " ; "
    $PROG -n 0 -b $I | grep "SG Entries" | awk '{print $5}'
done
