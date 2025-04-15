#!/bin/bash
rm -rf source
git clone https://github.com/coreutils/coreutils.git source
pushd source
  git checkout 68c5eec
  # for AFL argv fuzz
  sed -i '1283i #include "/root/projects/CPR/lib/argv-fuzz-inl.h"' src/split.c
  sed -i '1288i AFL_INIT_SET02("./split", "/root/projects/CPR/patches/extractfix/coreutils/gnubug-25003/dummy");' src/split.c
  # avoid writing out a lot of files during fuzzing
  sed -i '595i return false;' src/split.c
  # not bulding man pages
  sed -i '229d' Makefile.am
  # change gnulib source
  sed -i "s|git://git.sv.gnu.org/gnulib.git|https://github.com/coreutils/gnulib.git|g" .gitmodules
  sed -i "s|git://git.sv.gnu.org/gnulib|https://github.com/coreutils/gnulib.git|g" bootstrap
  ./bootstrap
popd

cp ./split.pacfix.c ./source/src/split.c

rm -rf smake_source && mkdir smake_source
pushd smake_source
  export FORCE_UNSAFE_CONFIGURE=1 && CC=clang CXX=clang++ ../source/configure
  CC=clang CXX=clang++ /root/projects/smake/smake --init
  CC=clang CXX=clang++ /root/projects/smake/smake CFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" CXXFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" -j 10
popd

rm -rf sparrow-out && mkdir sparrow-out
/root/projects/sparrow/bin/sparrow -outdir ./sparrow-out \
-frontend "clang" -unsound_alloc -unsound_const_string -unsound_recursion -unsound_noreturn_function \
-unsound_skip_global_array_init 1000 -skip_main_analysis -cut_cyclic_call -unwrap_alloc \
-entry_point "main" -max_pre_iter 10 -slice "bug=split.c:986" \
./smake_source/sparrow/src/split/*.i


cp split.orig.c source/src/split.c
rm -rf dafl_source && mkdir dafl_source
pushd dafl_source
  FORCE_UNSAFE_CONFIGURE=1 DAFL_SELECTIVE_COV="/root/projects/CPR/patches/extractfix/coreutils/gnubug-25003/sparrow-out/bug/slice_func.txt" \
  DAFL_DFG_SCORE="/root/projects/CPR/patches/extractfix/coreutils/gnubug-25003/sparrow-out/bug/slice_dfg.txt" \
  ASAN_OPTIONS=detect_leaks=0 CC=/root/projects/CLUDAFL/afl-clang-fast CXX=/root/projects/CLUDAFL/afl-clang-fast++ \
  ../source/configure

  DAFL_SELECTIVE_COV="/root/projects/CPR/patches/extractfix/coreutils/gnubug-25003/sparrow-out/bug/slice_func.txt" \
  DAFL_DFG_SCORE="/root/projects/CPR/patches/extractfix/coreutils/gnubug-25003/sparrow-out/bug/slice_dfg.txt" \
  ASAN_OPTIONS=detect_leaks=0 CC=/root/projects/CLUDAFL/afl-clang-fast CXX=/root/projects/CLUDAFL/afl-clang-fast++ \
  make CFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" CXXFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" -j 10
popd

rm -rf dafl-runtime && mkdir dafl-runtime
cp dafl_source/src/split dafl-runtime/split