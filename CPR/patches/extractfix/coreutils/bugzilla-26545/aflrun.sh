#!/bin/bash
rm -rf source
git clone https://github.com/coreutils/coreutils.git source
pushd source
  git checkout 8d34b45
  # for AFL argv fuzz
  sed -i '1215i #include "/root/projects/CPR/lib/argv-fuzz-inl.h"' src/shred.c
  sed -i '1220i AFL_INIT_SET03("./shred", "/home/yuntong/vulnfix/data/coreutils/gnubug_26545/dummy");' src/shred.c
  # -u option can cause a lot of files to be writting to disk during fuzzing; disable that
  sed -i '1260i break;' src/shred.c
  # remove and recreate output so that it does not grow too big.
  sed -i '1320i FILE* file_ptr = fopen(file[i], "w"); fclose(file_ptr);' src/shred.c
  # not bulding man pages
  sed -i '217d' Makefile.am
  # change gnulib source
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
  echo "shred.c:292" > $TMP_DIR/BBtargets.txt
  export AFLRUN_BB_TARGETS=$TMP_DIR/BBtargets.txt
  export AFLRUN_TARGETS="shred"
  export ADDITIONAL_FLAGS="-flto -fuse-ld=gold -Wl,-plugin-opt=save-temps"
  export FORCE_UNSAFE_CONFIGURE=1
  CC=$AFLRUN/afl-clang-lto CXX=$AFLRUN/afl-clang-lto++ ASAN_OPTIONS=detect_leaks=0 ../source/configure
  CC=$AFLRUN/afl-clang-lto CXX=$AFLRUN/afl-clang-lto++ ASAN_OPTIONS=detect_leaks=0 make CFLAGS="$ADDITIONAL_FLAGS -Wno-error -fsanitize=address -ggdb" CXXFLAGS="$ADDITIONAL_FLAGS -Wno-error -fsanitize=address -ggdb" LDFLAGS="-fsanitize=address" -j10
popd

rm -rf runtime && mkdir runtime
cp ./aflrun_build/src/shred runtime/shred.aflrun
