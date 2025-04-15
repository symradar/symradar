#!/bin/bash
rm -rf src
rm -rf patched && mkdir patched
git clone https://github.com/coreutils/coreutils.git src
pushd src
  git checkout 658529a
  git clone https://github.com/coreutils/gnulib.git
  ./bootstrap
  # patch
  cp ../make-prime-list.c src/make-prime-list.c
  FORCE_UNSAFE_CONFIGURE=1 LD=lld LD=lld CC=cpr-cc CXX=cpr-cxx ./configure CFLAGS='-g -O0 -fno-discard-value-names -static -fPIE' CXXFLAGS="$CFLAGS"
  make CFLAGS="-fno-discard-value-names -fPIC -fPIE -L/root/projects/uni-klee/build/lib  -lkleeRuntest" CXXFLAGS=$CFLAGS src/make-prime-list -j32
  # cp
  cp src/make-prime-list.c ../patched
  cp src/make-prime-list ../patched
popd
pushd patched
  extract-bc make-prime-list
popd
