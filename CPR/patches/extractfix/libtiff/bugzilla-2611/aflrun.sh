#!/bin/bash
rm -rf source
git clone https://github.com/vadz/libtiff.git source
pushd source
  git checkout 9a72a69
popd

export AFLRUN=/root/projects/AFLRun
rm -rf aflrun_build && mkdir aflrun_build
pushd aflrun_build
  mkdir temp
  TMP_DIR=$PWD/temp
  mkdir aflrun_tmp
  export AFLRUN_TMP=$PWD/aflrun_tmp
  echo "tif_ojpeg.c:816" > $TMP_DIR/BBtargets.txt
  export AFLRUN_BB_TARGETS=$TMP_DIR/BBtargets.txt
  export AFLRUN_TARGETS="tiffmedian"
  # export ADDITIONAL_FLAGS="-flto -fuse-ld=gold -Wl,-plugin-opt=save-temps"
  export FORCE_UNSAFE_CONFIGURE=1
  CC=$AFLRUN/afl-clang-lto CXX=$AFLRUN/afl-clang-lto++ ASAN_OPTIONS=detect_leaks=0 ../source/configure
  CC=$AFLRUN/afl-clang-lto CXX=$AFLRUN/afl-clang-lto++ ASAN_OPTIONS=detect_leaks=0 make CFLAGS="-static -fsanitize=address -fsanitize=undefined -g" CXXFLAGS="-static -fsanitize=address -fsanitize=undefined -g" -j10
popd

rm -rf runtime && mkdir runtime
cp aflrun_build/tools/tiffmedian ./runtime/tiffmedian.aflrun

