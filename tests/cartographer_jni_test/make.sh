#!/bin/sh

set -e
NATIVE_JAR='cartographer-platform-1.0.0-natives-linux.jar'
JAVA_LIB_JAR='cartographer-1.0.0.jar'
TEST_PATH='cartographerjar/jni/CartographerJavaTest.java'
TEST_CLASS_NAME='cartographerjar.jni.CartographerJavaTest'
GIT_ROOT_DIR=`git rev-parse --show-toplevel`
SCRIPT_PATH=$GIT_ROOT_DIR/tests/cartographer_jni_test

echo
echo 'Created by Eric Hähner'
echo 'https://github.com/thefangbear/JNI-By-Examples'
echo 'Options: ./make.sh'
echo 'Note: Cartographer JNI jars should be created and copied to this project before executing this script'
echo '--build                 extract jars and compile test java file'
echo '--execute-java          execute test java file'
echo '--cleanup               Cleanup build files'
echo

build_project () {
    rm -rf build
    mkdir -p build/natives/linux/
    cd build/natives/linux/
    jar xf $GIT_ROOT_DIR/$NATIVE_JAR
    rm -rf META-INF
    cd $SCRIPT_PATH/src
    javac -cp ".:$GIT_ROOT_DIR/$JAVA_LIB_JAR" -d ../ $TEST_PATH
    cd ../
    echo 'Build finished.'
}

execute_test () {
	# extract build files if not existing
	cd $GIT_ROOT_DIR/tests/test_data
	if [ ! -f "lrfdata.txt" ]; then
		tar xvzf data.tar.gz run1_lrf.txt run1_odo.txt
		mv run1_lrf.txt lrfdata.txt
		mv run1_odo.txt odomdata.txt
	fi
	cd $SCRIPT_PATH
    START=$(date +%s.%N)
    java -cp ".:$GIT_ROOT_DIR/$JAVA_LIB_JAR" $TEST_CLASS_NAME -Djava.library.path=$SCRIPT_PATH/build
    END=$(date +%s.%N)
    DIFF=$(echo "$END - $START" | bc)
    echo "time needed: $DIFF s"
    #plot
	gnuplot -e 'set palette defined ( -1 "#000000", 0 "#00FF00", 50 "#0000FF", 100 "#FF0000" ); set size ratio -1; plot "grid_java.txt" u 1:2:3 ls 5 lc palette z' -p
}

cleanup () {
    echo "deleted the following files:"
    rm -rf ./build
    echo "./build"
    rm -rf ./cartographerjar
    echo "./cartographerjar"
    find . -name "*.class" -type f
    find . -name "*.class" -type f -delete
}


# execute
if [ "$1" != "" ]; then
    while [ "$1" != "" ]; do
        case $1 in
            --build | -b                 )   build_project
                                              ;;
            --execute_test | -e          )   execute_test $2
                                              ;;
            --cleanup | -c               )   cleanup
        esac
        shift
    done
else
    cleanup
    build_project
    execute_test
fi
