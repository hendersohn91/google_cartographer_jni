#!/bin/sh
set -e
# Install the required libraries that are available as debs.
sudo apt-get update

# Install CMake 3.2 for Ubuntu Trusty and Debian Jessie.
sudo apt-get install lsb-release -y
if [[ "$(lsb_release -sc)" = "trusty" ]]
then
  sudo apt-get install cmake3 -y
elif [[ "$(lsb_release -sc)" = "jessie" ]]
then
  sudo sh -c "echo 'deb http://ftp.debian.org/debian jessie-backports main' >> /etc/apt/sources.list"
  sudo apt-get update
  sudo apt-get -t jessie-backports install cmake -y
else
  sudo apt-get install cmake -y
fi

sudo apt-get install -y \
    clang \
    g++ \
    git \
    google-mock \
    libboost-all-dev \
    libcairo2-dev \
    libcurl4-openssl-dev \
    libeigen3-dev \
    libgflags-dev \
    libgoogle-glog-dev \
    liblua5.2-dev \
    libsuitesparse-dev \
    ninja-build \
    python-sphinx

# create carto directory
mkdir cartographer_standalone
basedir=$(pwd)"/cartographer_standalone"
cd $basedir

# Build and install Ceres.
git clone https://ceres-solver.googlesource.com/ceres-solver
cd ceres-solver
git checkout tags/1.13.0
mkdir build
cd build
cmake .. -G Ninja -DCXX11=ON
ninja
CTEST_OUTPUT_ON_FAILURE=1 ninja test
sudo ninja install

# Build and install proto3.
cd $basedir
git clone https://github.com/google/protobuf.git
cd protobuf
git checkout tags/v3.4.1
mkdir build
cd build
cmake -G Ninja \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -Dprotobuf_BUILD_TESTS=OFF \
  ../cmake
ninja
sudo ninja install

# Build and install Cartographer.
cd $basedir
git clone https://github.com/googlecartographer/cartographer.git
cd cartographer
git checkout release-1.0
mkdir build
cd build
cmake .. -G Ninja
ninja
CTEST_OUTPUT_ON_FAILURE=1 ninja test
sudo ninja install
