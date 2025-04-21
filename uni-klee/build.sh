#!/bin/bash

cd thirdparty/minisat
mkdir build
cd build

cmake \
 -DCMAKE_BUILD_TYPE=Release \
 -DSTATIC_BINARIES=ON \
 -DBUILD_SHARED_LIBS=OFF \
 ..

make -j 32
make install
cd ../../..

cd thirdparty/stp
mkdir build
cd build

cmake \
 -DCMAKE_BUILD_TYPE=Release \
 -DENABLE_PYTHON_INTERFACE:BOOL=OFF \
 -DBUILD_SHARED_LIBS=OFF \
 -DTUNE_NATIVE:BOOL=ON \
 -DNO_BOOST=ON \
 ..

make -j 32
make install
cd ../../..

cd thirdparty/z3
# git clone https://github.com/Z3Prover/z3.git
# git checkout z3-4.8.12
mkdir build
cd build

cmake \
 -DCMAKE_BUILD_TYPE=Release \
 ..

make -j 32
make install
cd ../../..

cd thirdparty/klee-uclibc

./configure --make-llvm-lib --with-llvm-config="llvm-config-12"

make KLEE_CFLAGS="-DKLEE_SYM_PRINTF" -j 32

cd ../..

mkdir build
cd build

cmake \
 -DCMAKE_BUILD_TYPE=Debug \
 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
 -DCMAKE_CXX_FLAGS="-fno-rtti" \
 -DENABLE_TCMALLOC=ON \
 -DENABLE_SOLVER_STP=ON \
 -DENABLE_SOLVER_Z3=ON \
 -DENABLE_POSIX_RUNTIME=ON \
 -DENABLE_KLEE_UCLIBC=ON \
 -DKLEE_UCLIBC_PATH="../thirdparty/klee-uclibc" \
 -DENABLE_UNIT_TESTS=OFF \
 -DENABLE_SYSTEM_TESTS=OFF \
 -DCMAKE_C_COMPILER=gcc \
 -DCMAKE_CXX_COMPILER=g++ \
 ..

make -j 32
make install
