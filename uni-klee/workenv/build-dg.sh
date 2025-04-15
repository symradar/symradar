#! /bin/bash

DG_PATH="/dg"
LLVM_ROOT_PATH="/llvm-14"

cd $DG_PATH
mkdir build
cd build

# cmake -DLLVM_SRC_PATH=${LLVM_ROOT_PATH} \
#  -DLLVM_BUILD_PATH=${LLVM_ROOT_PATH}/build \
#  -DLLVM_DIR=${LLVM_ROOT_PATH}/build/lib/cmake \
#  ..
 cmake ..
make -j 4
