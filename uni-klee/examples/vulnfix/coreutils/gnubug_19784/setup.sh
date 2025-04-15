#!/bin/bash

mkdir -p patched
git clone https://github.com/coreutils/coreutils.git source
cd source/
git checkout 658529a

cp ../make-prime-list.c src/make-prime-list.c
git add src/make-prime-list.c
git commit -m "Apply patch"

./bootstrap
export FORCE_UNSAFE_CONFIGURE=1 && ./configure CFLAGS='-g -O0 -static -fPIE' CXXFLAGS="$CFLAGS" && make src/make-prime-list

cp src/make-prime-list ../patched
pushd ../patched
  cp ../exploit .
  cp ../Makefile .
popd