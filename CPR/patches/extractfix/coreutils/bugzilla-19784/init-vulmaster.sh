#!/bin/bash
rm -rf vulmaster-src
rm -rf vulmaster-patched && mkdir vulmaster-patched
git clone https://github.com/coreutils/coreutils.git vulmaster-src
pushd vulmaster-src
  git checkout 658529a
  git clone https://github.com/coreutils/gnulib.git
  ./bootstrap

  # patch
  cp ../vulmaster/make-prime-list.vulmaster-1.c src/make-prime-list.c
  FORCE_UNSAFE_CONFIGURE=1 LD=lld LD=lld CC=cpr-cc CXX=cpr-cxx ./configure CFLAGS='-g -O0 -fno-discard-value-names -static -fPIE' CXXFLAGS="$CFLAGS"
  make CFLAGS="-fno-discard-value-names -fPIC -fPIE -L/root/projects/uni-klee/build/lib  -lkleeRuntest" CXXFLAGS=$CFLAGS src/make-prime-list -j32
  # cp
  # cp vulmaster-src/make-prime-list.c ../patched
  cp src/make-prime-list ../vulmaster-patched/make-prime-list-1
popd
pushd vulmaster-patched
  extract-bc make-prime-list-1
popd
