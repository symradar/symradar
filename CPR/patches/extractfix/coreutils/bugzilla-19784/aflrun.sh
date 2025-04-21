#!/bin/bash
rm -rf source
git clone https://github.com/coreutils/coreutils.git source
pushd source
  git checkout 658529a
  # for AFL argv fuzz
  sed -i '29i #include "/root/projects/CPR/lib/argv-fuzz-inl.h"' src/make-prime-list.c
  sed -i '175i AFL_INIT_SET0("./make-prime-list");' src/make-prime-list.c
  git clone https://github.com/coreutils/gnulib.git
  ./bootstrap
popd

export AFLRUN=/root/projects/AFLRun
rm -rf aflrun_build && mkdir aflrun_build
pushd aflrun_build
  mkdir temp
  TMP_DIR=$PWD/temp
  mkdir aflrun_tmp
  export AFLRUN_TMP=$PWD/aflrun_tmp
  echo "make-prime-list.c:216" > $TMP_DIR/BBtargets.txt
  export AFLRUN_BB_TARGETS=$TMP_DIR/BBtargets.txt
  export AFLRUN_TARGETS="make-prime-list"
  # export ADDITIONAL_FLAGS="-flto -fuse-ld=gold -Wl,-plugin-opt=save-temps"
  export FORCE_UNSAFE_CONFIGURE=1
  CC=$AFLRUN/afl-clang-lto CXX=$AFLRUN/afl-clang-lto++ ASAN_OPTIONS=detect_leaks=0 ../source/configure
  CC=$AFLRUN/afl-clang-lto CXX=$AFLRUN/afl-clang-lto++ ASAN_OPTIONS=detect_leaks=0 make CFLAGS="-Wno-error -fsanitize=address -g" src/make-prime-list
popd

rm -rf runtime && mkdir runtime
cp aflrun_build/src/make-prime-list runtime/make-prime-list.aflrun
