#!/bin/bash
rm -rf dafl-src
rm -rf dafl-patched && mkdir dafl-patched
git clone https://github.com/coreutils/coreutils.git dafl-src
pushd concrete
  gcc -c -fpic -L. uni_klee_runtime_dafl.c
  gcc -shared -o libdafl_runtime.so uni_klee_runtime_dafl.o
  mv libdafl_runtime.so ../dafl-src
popd
pushd dafl-src
  git checkout 658529a
  git clone https://github.com/coreutils/gnulib.git
  ./bootstrap
  # patch
  cp ../make-prime-list.c src/make-prime-list.c
  FORCE_UNSAFE_CONFIGURE=1 ./configure CFLAGS='-g -O0 -static -fPIE -DDAFL_ASSERT -Wno-error -ldafl_runtime -L'${PWD}' -I'${PWD}'/../concrete' CXXFLAGS="$CFLAGS"
  make CFLAGS="-fPIC -fPIE -DDAFL_ASSERT -Wno-error -ldafl_runtime -L"${PWD}" -I"${PWD}"/../concrete" CXXFLAGS=$CFLAGS src/make-prime-list -j32
  # cp
  cp src/make-prime-list ../dafl-patched/bin
popd
