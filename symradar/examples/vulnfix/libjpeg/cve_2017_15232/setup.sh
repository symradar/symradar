#!/bin/bash

mkdir -p patched
git clone https://github.com/libjpeg-turbo/libjpeg-turbo.git
mv libjpeg-turbo source
cd source/
git checkout 3212005

cp ../jdpostct.c ./jdpostct.c
git add jdpostct.c
git commit -m "Apply patch"

autoreconf -fiv
CC=wllvm CXX=wllvm++ ./configure --without-simd CFLAGS="-static -O0 -g" CXXFLAGS="-static -O0 -g"
make CFLAGS="-static -O0 -g" CXXFLAGS="-static -O0 -g" -j10

cp ./djpeg ../patched
pushd ../patched
  cp ../Makefile .
  cp ../exploit .
popd
