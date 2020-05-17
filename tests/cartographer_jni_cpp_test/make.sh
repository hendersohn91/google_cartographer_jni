#!/bin/bash
set -e
GIT_ROOT_DIR=`git rev-parse --show-toplevel`
SCRIPT_PATH=$GIT_ROOT_DIR/tests/cartographer_jni_cpp_test
NATIVE_JAR='cartographer-platform-1.0.0-natives-linux.jar'
cd $SCRIPT_PATH
rm -rf build/
# extract build files if not existing
cd $GIT_ROOT_DIR/tests/test_data
if [ ! -f "lrfdata.txt" ]; then
	tar xvzf data.tar.gz run1_lrf.txt run1_odo.txt
	mv run1_lrf.txt lrfdata.txt
	mv run1_odo.txt odomdata.txt
fi
#build
cd $SCRIPT_PATH
if [ ! -d "build" ]; then
	mkdir build
fi
cd build
# extract necessary library
jar xvf $GIT_ROOT_DIR/$NATIVE_JAR
cmake ..
make

# execute
./cartographer_jni_cpp_test
#plot
gnuplot -e 'set palette defined ( -1 "#000000", 0 "#00FF00", 50 "#0000FF", 100 "#FF0000" ); set size ratio -1; plot "grid_cpp.txt" u 1:2:3 ls 5 lc palette z' -p

# paint grid with gnuplot
# data structure:
# x-value y-value grid-value
# unobserved values should have a low negative value for gnuplot
# commands:
# set palette defined ( -1 "#000000", 0 "#00FF00", 50 "#0000FF", 100 "#FF0000" )
# plot "grid.txt" u 1:2:3 ls 5 lc palette z
