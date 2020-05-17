# JNI for Google Cartographer

This project contains a java native interface to use google cartographer with Java. The code is tested at Ubuntu 18.04.2. The code was developed towards my master thesis at HTW-Dresden. For further documentation you can read this master thesis that is also contained in this repository.

# Build the JNI
```sh
$ cd src/
$ sh make.sh
```

The result are two jar files located at the root of the repository. The "cartographer-platform-1.0.0-natives-linux.jar" contains the C++ part and the "cartographer-1.0.0.jar" contains the Java part of the JNI.

# Tests for debugging
The repo also contains one test for the C++ Part only and another test for the complete JNI. The data used by the tests is located under tests/cartographer_jni_cpp_test/ and contains laser range data and odometry data.

## Test the C++ JNI Part
```sh
$ cd tests/cartographer_jni_cpp_test/
$ sh make.sh
```

## Test the complete JNI
```sh
$ cd tests/cartographer_jni_test/
$ sh make.sh
```

See [Master Thesis](https://github.com/hendersohn91)
# Restictions
- The JNI provides the generation of a 2D map only
- see Master Thesis for further information
