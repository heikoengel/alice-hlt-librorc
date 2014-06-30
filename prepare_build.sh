#!/bin/bash

VERSION=`cat CMakeLists.txt | grep "set(VERSION" | awk '{print $2}' | cut -d")" -f1 | cut -d"\"" -f2`
INSTALL_PATH="/opt/package/librorc/$VERSION/"
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
cmake -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_PATH -DCMAKE_BUILD_TYPE=Debug -DLIBMODE=$MODE ../../

cd $BASEDIR/build/sim_release
cmake -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_PATH -DCMAKE_BUILD_TYPE=Release -DMODELSIM=ON -DLIBMODE=$MODE ../../

cd $BASEDIR/build/sim_debug
cmake -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_PATH -DCMAKE_BUILD_TYPE=Debug -DMODELSIM=ON -DLIBMODE=$MODE ../../

cd $BASEDIR
