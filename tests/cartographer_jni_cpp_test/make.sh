#!/bin/bash
set -e
rm -rf build/
if [ ! -d "build" ]; then
	mkdir build
fi
cd build
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
