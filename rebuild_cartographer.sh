#!/bin/sh
cd cartographer
#rm -rf build
mkdir build
cd build
cmake .. -G Ninja
ninja
CTEST_OUTPUT_ON_FAILURE=1 ninja test
sudo ninja install
