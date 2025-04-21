#!/bin/bash

mkdir -p patched
git clone git://sourceware.org/git/binutils-gdb.git
mv binutils-gdb source
cd source/
git checkout 53f7e8ea7fad1fcff1b58f4cbd74e192e0bcbc1d

cp ../readelf.c binutils/readelf.c
git add binutils/readelf.c
git commit -m 'Apply patch'

CC=wllvm CXX=wllvm++ CFLAGS="-g -O0" CXXFLAGS="$CFLAGS" ./configure --disable-shared --disable-gdb --disable-libdecnumber --disable-readline --disable-sim --disable-werror
make -j32

cp binutils/readelf ../patched

pushd ../patched
  cp ../exploit .
  cp ../Makefile .
popd
