#!/bin/bash
CPR_DIR=${CPR_DIR:-/root/projects/CPR}
PATH=$PATH:$CPR_DIR/tools
rm -rf source
project_url=https://github.com/vadz/libtiff.git
commit_id=f3069a5
patched_dir=tools
patched_file=tiff2ps.c
bin_dir=tools
bin_file=tiff2ps
git clone $project_url source
pushd source
  git checkout $commit_id
popd

cp tiff2ps.pacfix.c ./source/tools/tiff2ps.c

rm -rf smake_source && mkdir smake_source
pushd smake_source
  CC=clang CXX=clang++ ../source/configure
  CC=clang CXX=clang++ /root/projects/smake/smake --init
  CC=clang CXX=clang++ /root/projects/smake/smake CFLAGS="-static -fsanitize=address -fsanitize=undefined -g" CXXFLAGS="-static -fsanitize=address -fsanitize=undefined -g" -j10
popd

rm -rf sparrow-out && mkdir sparrow-out
/root/projects/sparrow/bin/sparrow -outdir ./sparrow-out \
-frontend "cil" -unsound_alloc -unsound_const_string -unsound_recursion -unsound_noreturn_function \
-unsound_skip_global_array_init 1000 -skip_main_analysis -cut_cyclic_call -unwrap_alloc \
-entry_point "main" -max_pre_iter 10 -slice "bug=tiff2ps.c:2437" \
./smake_source/sparrow/tools/tiff2ps/*.i

cp tiff2ps.orig.c ./source/tools/tiff2ps.c

rm -rf dafl_source && mkdir dafl_source
pushd dafl_source
  DAFL_SELECTIVE_COV="/root/projects/CPR/patches/extractfix/libtiff/bugzilla-2633/sparrow-out/bug/slice_func.txt" \
  DAFL_DFG_SCORE="/root/projects/CPR/patches/extractfix/libtiff/bugzilla-2633/sparrow-out/bug/slice_dfg.txt" \
  ASAN_OPTIONS=detect_leaks=0 CC=/root/projects/CLUDAFL/afl-clang-fast CXX=/root/projects/CLUDAFL/afl-clang-fast++ \
  CMAKE_EXPORT_COMPILE_COMMANDS=1 CFLAGS="-DFORTIFY_SOURCE=2 -fno-omit-frame-pointer -fsanitize=address -ggdb -Wno-error" \
  CXXFLAGS="$CFLAGS" ../source/configure

  DAFL_SELECTIVE_COV="/root/projects/CPR/patches/extractfix/libtiff/bugzilla-2633/sparrow-out/bug/slice_func.txt" \
  DAFL_DFG_SCORE="/root/projects/CPR/patches/extractfix/libtiff/bugzilla-2633/sparrow-out/bug/slice_dfg.txt" \
  ASAN_OPTIONS=detect_leaks=0 CC=/root/projects/CLUDAFL/afl-clang-fast CXX=/root/projects/CLUDAFL/afl-clang-fast++ \
  make CFLAGS="-static -fsanitize=address -fsanitize=undefined -g" CXXFLAGS="-static -fsanitize=address -fsanitize=undefined -g" -j10
popd

rm -rf dafl-runtime && mkdir dafl-runtime
cp dafl_source/tools/tiffcrop dafl-runtime/tiffcrop

# AFL_NO_UI=1 timeout 12h /root/projects/DAFL/afl-fuzz -C -t 2000ms -m none -i ./in -p /home/yuntong/vulnfix/data/libtiff/cve_2016_5321/sparrow-out/bug/slice_dfg.txt -o 2024-04-04-test -- ./tiffcrop @@ /tmp/out.tmp

