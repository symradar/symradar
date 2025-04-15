#!/bin/bash
CPR_DIR=${CPR_DIR:-/root/projects/CPR}
PATH=$PATH:$CPR_DIR/tools
LIB_DIR=$CPR_DIR/lib
rm -rf val-src val-runtime
mkdir -p val-runtime
project_url=https://github.com/coreutils/coreutils.git
commit_id=68c5eec
patched_dir=src
patched_file=split.c
pc_file=split.val.c
bin_dir=src
bin_file=split
git clone $project_url val-src
pushd val-src
  git checkout $commit_id
  # Build
  git clone https://github.com/coreutils/gnulib.git
  sed -i '229d' Makefile.am
  ./bootstrap
    # Patch
  cp ../${pc_file} ${patched_dir}/${patched_file}
  rm -rf build
  mkdir build
  pushd build
    FORCE_UNSAFE_CONFIGURE=1 ../configure CFLAGS="-g -O0 -static -fPIE -fPIC" CXXFLAGS="$CFLAGS" --disable-silent-rules
    make CFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" CXXFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" -j 10
  popd
  # cp
  cp ${patched_dir}/${patched_file} ../val-runtime
  cp build/${bin_dir}/${bin_file} ../val-runtime
popd

pushd val-runtime
  extract-bc ${bin_file}
  llvm-dis ${bin_file}.bc
popd