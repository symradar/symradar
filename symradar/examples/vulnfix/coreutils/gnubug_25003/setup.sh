#!/bin/bash

mkdir -p patched
git clone https://github.com/coreutils/coreutils.git source
cd source/
git checkout 68c5eec

cp ../split.c src/split.c
sed -i '229d' Makefile.am

./bootstrap
export FORCE_UNSAFE_CONFIGURE=1 && CC=wllvm CXX=wllvm++ ./configure CFLAGS='-g -O0 -static -fPIE' CXXFLAGS="$CFLAGS"
make -j32

cp src/split ../patched
pushd ../patched
  cp ../Makefile .
  cp ../exploit .
popd
