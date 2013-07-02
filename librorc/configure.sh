#!/bin/bash

mkdir build
mkdir build/debug
mkdir build/release

cd build/release
cmake -DCMAKE_BUILD_TYPE=Release ../../
cmake -DCMAKE_BUILD_TYPE=Release ../../

cd ../../

cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../../
cmake -DCMAKE_BUILD_TYPE=Debug ../../
