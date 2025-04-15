#!/bin/bash
CPR_DIR=${CPR_DIR:-/root/projects/CPR}
PATH=$PATH:$CPR_DIR/tools
rm -rf src patched
mkdir -p patched
project_url=https://github.com/coreutils/coreutils.git
commit_id=ca99c52
patched_dir=src
patched_file=pr.c
bin_dir=src
bin_file=pr
git clone $project_url src
pushd src
  git checkout $commit_id
  git clone https://github.com/coreutils/gnulib.git
  # Build
  ./bootstrap
  # Patch
  cp ../${patched_file} ${patched_dir}/${patched_file}
  rm -rf build
  mkdir build
  pushd build
    FORCE_UNSAFE_CONFIGURE=1 LD=lld CC=cpr-cc CXX=cpr-cxx ../configure CFLAGS='-g -O0 -fno-discard-value-names -static -fPIE -fPIC' CXXFLAGS="$CFLAGS"
    make CFLAGS="-fno-discard-value-names -fPIC -fPIE -L/root/projects/uni-klee/build/lib  -lkleeRuntest" CXXFLAGS=$CFLAGS -j32
  popd
  # cp
  cp ${patched_dir}/${patched_file} ../patched
  cp build/${bin_dir}/${bin_file} ../patched
popd
pushd patched
  cp ../exploit.txt .
  extract-bc ${bin_file}
popd
