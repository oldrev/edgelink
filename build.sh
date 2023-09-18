#!/bin/bash

set -e
set -x

BASEDIR=$(dirname "$0")
pushd "$BASEDIR"

mkdir -p build

<<<<<<< HEAD
#conan install . --output-folder=build --build=missing --settings=build_type=Debug --settings=compiler.cppstd=23
#source ./build/conanbuild.sh
=======
conan install . --output-folder=build --build=missing --settings=build_type=Debug --settings=compiler.cppstd=23 -pr profiles/x86_64-linux
source ./build/conanbuild.sh
>>>>>>> 2e7c5f0745e854c668eee7da9d4ef10eedb2607d
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build