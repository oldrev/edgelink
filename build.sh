#!/bin/bash

set -e
set -x

BASEDIR=$(dirname "$0")
pushd "$BASEDIR"

mkdir -p build

conan install . --output-folder=build --build=missing --settings=build_type=Debug -pr profiles/x86_64-linux.ini
source ./build/conanbuild.sh
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build