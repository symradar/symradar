#!/bin/bash
CPR_DIR=${CPR_DIR:-/root/projects/CPR}
PATH=$PATH:$CPR_DIR/tools
LIB_DIR=$CPR_DIR/lib
rm -rf val-src val-runtime
mkdir -p val-runtime
project_url=https://github.com/vadz/libtiff.git
commit_id=f3069a5
patched_dir=tools
patched_file=tiff2ps.c
bin_dir=tools
bin_file=tiff2ps
git clone $project_url val-src
pushd val-src
  git checkout $commit_id
  # Patch
  cp ../tiff2ps.val.c ${patched_dir}/${patched_file}
  ./autogen.sh
  LD=lld CC=wllvm CXX=wllvm++ ./configure --enable-static --disable-shared --without-threads --without-lzma
  CC=wllvm CXX=wllvm++ make CFLAGS="-static -O0 -g -fno-discard-value-names -L$LIB_DIR -luni_klee_memory_check" CXXFLAGS="-static -O0 -g -fno-discard-value-names -L$LIB_DIR -luni_klee_memory_check" -j16
  # cp
  cp ${patched_dir}/${patched_file} ../val-runtime
  cp ${bin_dir}/${bin_file} ../val-runtime
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