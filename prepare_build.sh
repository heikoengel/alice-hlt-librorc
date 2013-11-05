#!/bin/bash

INSTALL_PATH=/home/weiler/opt/librorc/

MODE="SHARED"
#MODE="STATIC"

BASEDIR=`pwd`/`dirname $0`
echo $BASEDIR

mkdir -p $BASEDIR/build
mkdir -p $BASEDIR/build/debug
mkdir -p $BASEDIR/build/release
mkdir -p $BASEDIR/build/sim_debug
mkdir -p $BASEDIR/build/sim_release

cd $BASEDIR/build/release
cmake -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_PATH -DCMAKE_BUILD_TYPE=Release -DLIBMODE=$MODE ../../

cd $BASEDIR/build/debug
cmake -DCMAKE_BUILD_TYPE=Debug -DLIBMODE=$MODE ../../

cd $BASEDIR/build/sim_release
cmake -DCMAKE_BUILD_TYPE=Release -DSIM=ON -DLIBMODE=$MODE ../../

cd $BASEDIR/build/sim_debug
cmake -DCMAKE_BUILD_TYPE=Debug -DSIM=ON -DLIBMODE=$MODE ../../

cd $BASEDIR
