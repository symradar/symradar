#!/bin/bash
CPR_DIR=${CPR_DIR:-/root/projects/CPR}
PATH=$PATH:$CPR_DIR/tools
LIB_DIR=$CPR_DIR/lib
rm -rf val-src val-runtime
mkdir -p val-runtime
project_url=https://github.com/coreutils/coreutils.git
commit_id=ca99c52
patched_dir=src
patched_file=pr.c
bin_dir=src
bin_file=pr
git clone $project_url val-src
pushd val-src
  git checkout $commit_id
  git clone https://github.com/coreutils/gnulib.git
  ./bootstrap
  # Patch
  cp ../pr.val.c ${patched_dir}/${patched_file}
  rm -rf build
  mkdir build
  pushd build
    FORCE_UNSAFE_CONFIGURE=1 CC=wllvm CXX=wllvm++ ../configure CFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g -L$LIB_DIR -luni_klee_memory_check" CXXFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g -L$LIB_DIR -luni_klee_memory_check"
    make  -j32
  popd
  # cp
  cp ${patched_dir}/${patched_file} ../val-runtime
  cp build/${bin_dir}/${bin_file} ../val-runtime
popd

pushd val-runtime
  extract-bc ${bin_file} -o ${bin_file}.ng.bc
  llvm-dis ${bin_file}.ng.bc
  export UNI_KLEE_SYMBOLIC_GLOBALS_FILE="${UNI_KLEE_SYMBOLIC_GLOBALS_FILE_OVERRIDE:-$PWD/../patched/2025-03-10-high-0/base-mem.symbolic-globals}"
  opt -load $LIB_DIR/UniKleeGlobalVariablePass.so -global-var-pass < ${bin_file}.ng.bc > ${bin_file}.g.bc
  llc -filetype=obj ${bin_file}.g.bc -o ${bin_file}.g.o
  clang ${bin_file}.g.o -o ${bin_file}.g -ljpeg -llzma -lz -fsanitize=address -fsanitize=undefined
  cp ${bin_file}.g ${bin_file}
popd