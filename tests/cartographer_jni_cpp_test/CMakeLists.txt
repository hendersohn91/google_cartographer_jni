cmake_minimum_required(VERSION 3.10)
project(cartographer_jni_cpp_test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
set(CMAKE_CXX_STANDARD 11)
link_directories(/usr/local/lib/absl/ /usr/lib/x86_64-linux-gnu/ /usr/local/lib/)

add_executable(cartographer_jni_cpp_test jni_main.cpp)

include_directories(/usr/local/include)
include_directories(/usr/include/eigen3/)
include_directories(/usr/include/lua5.2/)
include_directories(build/)


find_library(CARTOGRAPHER_JNI_LIB cartographer_native_interface build/)
target_link_libraries(cartographer_jni_cpp_test cartographer protobuf protobuf-lite protoc glog pthread ceres boost_iostreams lua5.2 cholmod blas lapack cxsparse cairo ${CARTOGRAPHER_JNI_LIB})
