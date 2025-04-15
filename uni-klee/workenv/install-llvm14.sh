#! /bin/bash

ROOT_PATH="$1"
VERSION="14.0.6"
WHOLE_PATH="${ROOT_PATH}/llvm-14"
echo $WHOLE_PATH

wget https://github.com/llvm/llvm-project/releases/download/llvmorg-${VERSION}/llvm-${VERSION}.src.tar.xz
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-${VERSION}/clang-${VERSION}.src.tar.xz
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-${VERSION}/compiler-rt-${VERSION}.src.tar.xz
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-${VERSION}/clang-tools-extra-${VERSION}.src.tar.xz

tar -xvf llvm-${VERSION}.src.tar.xz
tar -xvf clang-${VERSION}.src.tar.xz
tar -xvf compiler-rt-${VERSION}.src.tar.xz
tar -xvf clang-tools-extra-${VERSION}.src.tar.xz

mv llvm-${VERSION}.src ${WHOLE_PATH}
mv clang-${VERSION}.src ${WHOLE_PATH}/tools/clang
mv clang-tools-extra-${VERSION}.src ${WHOLE_PATH}/tools/clang/tools/clang-tools-extra
mv compiler-rt-${VERSION}.src ${WHOLE_PATH}/projects/compiler-rt

cd $WHOLE_PATH
mkdir build
cd build
cmake -G Ninja -DLLVM_INCLUDE_BENCHMARKS=OFF ..
ninja -j4 