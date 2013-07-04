#!/bin/bash

BASEDIR=`pwd`/`dirname $0`
echo $BASEDIR

mkdir $BASEDIR/build
mkdir $BASEDIR/build/debug
mkdir $BASEDIR/build/release
mkdir $BASEDIR/build/sim_debug
mkdir $BASEDIR/build/sim_release

cd $BASEDIR/build/release
cmake -V -DCMAKE_BUILD_TYPE=Release ../../

cd $BASEDIR/build/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../../

cd $BASEDIR/build/sim_release
cmake -DCMAKE_BUILD_TYPE=Release -DSIM=ON ../../

cd $BASEDIR/build/sim_debug
cmake -DCMAKE_BUILD_TYPE=Debug -DSIM=ON ../../

cd $BASEDIR
