#! /bin/bash

cd /klee
mkdir build
cd ./build

cp -r /stp/source/deps/minisat /klee/build/deps/minisat

cmake \
 -DCMAKE_BUILD_TYPE=Debug \
 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
 -DCMAKE_CXX_FLAGS="-fno-rtti" \
 -DENABLE_TCMALLOC=ON \
 -DENABLE_SOLVER_STP=ON \
 -DENABLE_SOLVER_Z3=ON \
 -DENABLE_POSIX_RUNTIME=ON \
 -DENABLE_KLEE_UCLIBC=ON \
 -DKLEE_UCLIBC_PATH="/uclibc" \
 -DENABLE_UNIT_TESTS=OFF \
 -DENABLE_SYSTEM_TESTS=OFF \
 -DENABLE_DG=ON \
 -DDG_PATH="/dg" \
 ..

make -j 32