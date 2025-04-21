#!/bin/bash
CPR_DIR=${CPR_DIR:-/root/projects/CPR}
PATH=$PATH:$CPR_DIR/tools
LIB_DIR=$CPR_DIR/lib
rm -rf val-src val-runtime
mkdir -p val-runtime
project_url=https://github.com/coreutils/coreutils.git
commit_id=8d34b45
patched_dir=src
patched_file=shred.c
bin_dir=src
bin_file=shred
git clone $project_url val-src
pushd val-src
  git checkout $commit_id
  git clone https://github.com/coreutils/gnulib.git
  # Build
  sed -i '217d' Makefile.am
  ./bootstrap
    # Patch
  cp ../shred.val.c ${patched_dir}/${patched_file}
  rm -rf build
  mkdir build
  pushd build
    FORCE_UNSAFE_CONFIGURE=1 CC=wllvm CXX=wllvm++ ../configure CFLAGS="-g -O0 -fno-discard-value-names -static -fPIE -fPIC -L$LIB_DIR -luni_klee_memory_check" CXXFLAGS="$CFLAGS"
    make -j32
  popd
  # cp
  cp ${patched_dir}/${patched_file} ../val-runtime
  cp build/${bin_dir}/${bin_file} ../val-runtime
popd
pushd patched
  cp ../exploit.txt .
  extract-bc ${bin_file}
popd
