#!/bin/bash

mkdir -p patched
unzip source.zip
cd source/

# Patch
cp ../tiffcrop.c tools/tiffcrop.c

# Remove longjmp calls
sed -i '118d;221d' libtiff/tif_jpeg.c
sed -i '153d;2429d' libtiff/tif_ojpeg.c

./autogen.sh
CC=wllvm CXX=wllvm++ ./configure --enable-static --disable-shared --without-threads --without-lzma
CC=wllvm CXX=wllvm++ make CFLAGS="-static -O0 -g" CXXFLAGS="-static -O0 -g" -j10

cp tools/tiffcrop.c ../patched
cp tools/tiffcrop ../patched
pushd ../patched
  cp ../exploit .
  cp ../Makefile .
popd
