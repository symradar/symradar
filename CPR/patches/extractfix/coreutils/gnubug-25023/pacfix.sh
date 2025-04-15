#!/bin/bash
rm -rf source
git clone https://github.com/coreutils/coreutils.git source
pushd source
  git checkout ca99c52
  # for AFL argv fuzz
  sed -i '856i #include "/root/projects/CPR/lib/argv-fuzz-inl.h"' src/pr.c
  sed -i '860i AFL_INIT_SET0234("./pr", "/root/projects/CPR/patches/extractfix/coreutils/gnubug-25003/dummy", "-m", "/root/projects/CPR/patches/extractfix/coreutils/gnubug-25003/dummy");' src/pr.c
  # not bulding man pages
  sed -i '229d' Makefile.am
  # change gnulib source
  sed -i "s|git://git.sv.gnu.org/gnulib.git|https://github.com/coreutils/gnulib.git|g" .gitmodules
  sed -i "s|git://git.sv.gnu.org/gnulib|https://github.com/coreutils/gnulib.git|g" bootstrap
  ./bootstrap
popd

rm -rf pacfix
cp -r source pacfix
pushd pacfix
  export FORCE_UNSAFE_CONFIGURE=1 && ./configure CFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" CXXFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g"
  make CFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" CXXFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" -j10
  gcc  -E -fno-optimize-sibling-calls -fno-strict-aliasing -fno-asm -std=c99 -I. -I./lib  -Ilib -I./lib -Isrc -I./src -Wno-error -fsanitize=address -fsanitize=undefined -g -c src/pr.c -lm -s > src/pr.c.i
  cilly --domakeCFG --gcc=/usr/bin/gcc-7 --out=tmp.c ./src/pr.c.i
  mv tmp.c ./src/pr.c.i.c 
  cp ./src/pr.c.i.c src/pr.c
popd
/root/projects/pacfix/main.exe -uniklee -lv_only 1 config

cp ./pr.pacfix.c ./source/src/pr.c 


rm -rf smake_source && mkdir smake_source
pushd smake_source
  export FORCE_UNSAFE_CONFIGURE=1 && CC=clang CXX=clang++ ../source/configure CFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" CXXFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g"
  CC=clang CXX=clang++ /root/projects/smake/smake --init
  CC=clang CXX=clang++ /root/projects/smake/smake CFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" CXXFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" -j10
popd

rm -rf sparrow-out && mkdir sparrow-out
/root/projects/sparrow/bin/sparrow -outdir ./sparrow-out \
-frontend "clang" -unsound_alloc -unsound_const_string -unsound_recursion -unsound_noreturn_function \
-unsound_skip_global_array_init 1000 -skip_main_analysis -cut_cyclic_call -unwrap_alloc \
-entry_point "main" -max_pre_iter 10 -slice "bug=pr.c:1197" \
./smake_source/sparrow/src/pr/*.i

rm -rf dafl_source && mkdir dafl_source
pushd dafl_source
  DAFL_SELECTIVE_COV="/root/projects/CPR/patches/extractfix/coreutils/gnubug-25023/sparrow-out/bug/slice_func.txt" \
  DAFL_DFG_SCORE="/root/projects/CPR/patches/extractfix/coreutils/gnubug-25023/sparrow-out/bug/slice_dfg.txt" \
  ASAN_OPTIONS=detect_leaks=0 CC=/root/projects/CLUDAFL/afl-clang-fast CXX=/root/projects/CLUDAFL/afl-clang-fast++ \
  ../source/configure

  DAFL_SELECTIVE_COV="/root/projects/CPR/patches/extractfix/coreutils/gnubug-25023/sparrow-out/bug/slice_func.txt" \
  DAFL_DFG_SCORE="/root/projects/CPR/patches/extractfix/coreutils/gnubug-25023/sparrow-out/bug/slice_dfg.txt" \
  ASAN_OPTIONS=detect_leaks=0 CC=/root/projects/CLUDAFL/afl-clang-fast CXX=/root/projects/CLUDAFL/afl-clang-fast++ \
  make CFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" CXXFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" -j 10
popd

rm -rf dafl-runtime && mkdir dafl-runtime
cp dafl_source/src/pr dafl-runtime/pr
