#!/bin/sh
set -e
NATIVE_JAR='cartographer-platform-1.0.0-natives-linux.jar'
JAVA_LIB_JAR='cartographer-1.0.0.jar'
JAVA_BRIDGE_PATH='cartographer/jni/CartographerJniJavaBridge.java'
JAVA_BRIDGE_PATH_CLASS='cartographer/jni/CartographerJniJavaBridge.class'
JAVA_BRIDGE_CLASS_NAME='cartographer.jni.CartographerJniJavaBridge'
# find java version
JAVA_VER=$(java -version 2>&1 | sed -n ';s/.* version "\(.*\)\.\(.*\)\..*".*/\1\2/p;')

echo
echo 'Created by Rui-Jie Fang, modified by Eric Hähner'
echo 'https://github.com/thefangbear/JNI-By-Examples'
echo 'Options: ./make.sh'
echo '--create-cpp-header -ch     Create the header cpp file from the java file'
echo '--build -b                  cmake, compile java and generate two jars (native lib and java lib)'
echo '--cleanup -cu               Cleanup build files'
echo

if [ -z "$JAVA_HOME"]; then
    echo 'No JAVA_HOME variable is set! DO IT NOW!'
    exit 1
fi

create_cpp_header () {
    if [ "$JAVA_VER" -ge 18 ];then
        javac $JAVA_BRIDGE_PATH -h .
    else
        javah $JAVA_BRIDGE_CLASS_NAME
    fi
    echo "Generation finished."
}

build_project () {
    curdir=`pwd`
    cd ../
    rm -rf build
    mkdir build
    cd build
    cmake $curdir
    make
    jar cvf $NATIVE_JAR libcartographer_native_interface.so -C ../src/ cartographer_jni_cpp_bridge.h
    mv $NATIVE_JAR ../
    cd $curdir
    javac $JAVA_BRIDGE_PATH
    jar cvf $JAVA_LIB_JAR $JAVA_BRIDGE_PATH_CLASS
    mv $JAVA_LIB_JAR ../
    echo "created $JAVA_LIB_JAR and $NATIVE_JAR at $(cd ../ && pwd)"
    echo 'Build finished.'
}

cleanup () {
    echo "deleted the following files:"
    find . -name "*.class" -type f
    find . -name "*.class" -type f -delete
    find . -name "*.jar" -type f
    find . -name "*.jar" -type f -delete
    rm -rf ../cmake-build-debug
    echo "../cmake-build-debug"
    rm -rf ../build
    echo "../build"
}


# execute
if [ "$1" != "" ]; then
    while [ "$1" != "" ]; do
        case $1 in
            --create-cpp-header | -ch       )   create_cpp_header
                                        ;;
            --build | -b                 )   build_project
                                        ;;
            --cleanup | -cu )   cleanup
        esac
        shift
    done
else
    cleanup
    create_cpp_header
    build_project
fi
