#!/bin/bash
CPR_DIR=${CPR_DIR:-/root/projects/CPR}
PATH=$PATH:$CPR_DIR/tools
rm -rf dafl-src dafl-patched
mkdir -p dafl-patched
project_url=https://github.com/coreutils/coreutils.git
commit_id=8d34b45
patched_dir=src
patched_file=shred.c
bin_dir=src
bin_file=shred
git clone $project_url dafl-src
pushd concrete
  gcc -c -fpic -L. uni_klee_runtime_dafl.c
  gcc -shared -o libdafl_runtime.so uni_klee_runtime_dafl.o
  mv libdafl_runtime.so ../dafl-src
popd
pushd dafl-src
  git checkout $commit_id
  git clone https://github.com/coreutils/gnulib.git
  # Build
  ./bootstrap
    # Patch
  cp ../${patched_file} ${patched_dir}/${patched_file}
  rm -rf build
  mkdir build
  pushd build
    FORCE_UNSAFE_CONFIGURE=1 ../configure CFLAGS='-g -O0 -static -fPIE -fPIC -DDAFL_ASSERT -Wno-error' CXXFLAGS="$CFLAGS"
    make CFLAGS="-fPIC -fPIE -ldafl_runtime -L"${PWD}/.." -I"${PWD}"/../../concrete -DDAFL_ASSERT -Wno-error" CXXFLAGS=$CFLAGS -j32
  popd
  # cp
  cp build/${bin_dir}/${bin_file} ../dafl-patched/bin
popd
