#!/bin/bash

mkdir build
mkdir build/debug
mkdir build/release
mkdir build/sim_debug
mkdir build/sim_release

cd build/release
cmake -DCMAKE_BUILD_TYPE=Release ../../

cd ../../

cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../../

cd ../../

cd build/sim_debug
cmake -DCMAKE_BUILD_TYPE=Debug -DSIM=ON ../../
