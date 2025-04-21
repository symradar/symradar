#!/bin/bash
mkdir -p patched

unzip source.zip
cd source/

# Apply patch
cp ../bitmap_io.c ./src/bitmap_io.c

./configure CC=wllvm CXX=wllvm++
make CFLAGS="-static -O0 -g -fno-builtin" CXXFLAGS="-static -O0 -g -fno-builtin" -j10

cp src/potrace ../patched

pushd ../patched
  cp ../exploit .
  cp ../Makefile .
popd