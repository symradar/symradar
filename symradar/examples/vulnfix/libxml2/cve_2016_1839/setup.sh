#!/bin/bash

mkdir -p patched
git clone https://gitlab.gnome.org/GNOME/libxml2.git
mv libxml2 source
cd source/
git checkout db07dd61

cp ../HTMLparser.c ./HTMLparser.c
git add HTMLparser.c
git commit -m "Apply patch"

./autogen.sh
CC=wllvm CXX=wllvm++ make CFLAGS="-static -O0 -g" CXXFLAGS="-static -O0 -g" -j10

cp ./xmllint ../patched
pushd ../patched
  cp ../exploit .
  cp ../Makefile .
popd
