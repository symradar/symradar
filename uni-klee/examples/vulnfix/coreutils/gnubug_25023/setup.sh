#!/bin/bash
mkdir -p patched
git clone https://github.com/coreutils/coreutils.git source
cd source/
git checkout ca99c52

# for AFL argv fuzz
# sed -i '856i #include "/home/yuntong/vulnfix/thirdparty/AFL/experimental/argv_fuzzing/argv-fuzz-inl.h"' src/pr.c
# sed -i '860i AFL_INIT_SET0234("./pr", "/home/yuntong/vulnfix/data/coreutils/gnubug_25023/dummy", "-m", "/home/yuntong/vulnfix/data/coreutils/gnubug_25023/dummy");' src/pr.c
# not bulding man pages
sed -i '229d' Makefile.am

./bootstrap
export FORCE_UNSAFE_CONFIGURE=1 && ./configure CC="wllvm" CXX="wllvm++" --disable-nls CFLAGS="-Wno-error -O0 -g --rtlib=compiler-rt -Ddisable_overflow -Xclang-disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__" CXXFLAGS="-Wno-error -O0 -g --rtlib=compiler-rt -Ddisable_overflow -Xclang-disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__"
# make CFLAGS="-Wno-error -O0 -g" CXXFLAGS="-Wno-error -O0 -g" -j10
make CC="wllvm" CXX="wllvm++" CFLGAS="-static -g -O0 -fno-builtin -DJPEG_SUPPORT=true" -j 32
cp src/pr ../patched
cd ../patched
cp ../exploit .