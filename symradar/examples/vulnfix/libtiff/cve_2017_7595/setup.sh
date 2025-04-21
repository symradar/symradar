#!/bin/bash

mkdir -p patched

git clone https://github.com/vadz/libtiff.git
mv libtiff source
cd source/
git checkout 2c00d31

cp ../tif_jpeg.c libtiff/tif_jpeg.c
git add libtiff/tif_jpeg.c
git commit -m 'Apply patch'

sed -i '153d;2463d' libtiff/tif_ojpeg.c
git add libtiff/tif_ojpeg.c
git commit -m 'Remove longjmp calls'


./configure
make CC="wllvm" CFLAGS="-static -g -O0 -fno-builtin -DJPEG_SUPPORT=true" CXXFLAGS="-static -g -O0 -fno-builtin  -DJPEG_SUPPORT=true" -j32

cp tools/tiffcp ../patched
cp libtiff/tif_jpeg.c ../patched
pushd ../patched
  cp ../exploit .
  cp ../Makefile .
popd
