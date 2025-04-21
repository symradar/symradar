#!/bin/bash

mkdir patched

git clone https://github.com/libming/libming.git
mv libming source
cd source/
git checkout cc6a386

cp ../listmp3.c util/listmp3.c
git add util/listmp3.c
git commit -m "Apply patch"

./autogen.sh
./configure --disable-freetype
make CC="wllvm" CFLAGS="-static -g -O0" CXXFLAGS="-static -g -O0"

cp util/listmp3 ../patched
cp util/listmp3.c ../patched
pushd ../uni-klee
  cp ../exploit .
  cp ../Makefile .
popd
