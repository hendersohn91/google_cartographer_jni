#!/bin/bash
set -e
GIT_ROOT_DIR=`git rev-parse --show-toplevel`
SCRIPT_PATH=$GIT_ROOT_DIR/tests/cartographer_jni_cpp_test
NATIVE_JAR='cartographer-platform-1.0.0-natives-linux.jar'
cd $SCRIPT_PATH
rm -rf build/
if [ ! -d "build" ]; then
	mkdir build
fi
cd build
jar xvf $GIT_ROOT_DIR/$NATIVE_JAR
cmake ..
make
./cartographer_jni_cpp_test

# paint grid with gnuplot
# data structure:
# x-value y-value grid-value
# unobserved values should have a low negative value for gnuplot
# commands:
# set palette defined ( -1 "#000000", 0 "#00FF00", 50 "#0000FF", 100 "#FF0000" )
# plot "grid.txt" u 1:2:3 ls 5 lc palette z
