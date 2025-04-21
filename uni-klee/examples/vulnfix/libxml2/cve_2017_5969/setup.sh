#!/bin/bash

mkdir -p patched

git clone https://gitlab.gnome.org/GNOME/libxml2.git
mv libxml2 source
cd source/
git checkout 362b3229

cp ../valid.c ./valid.c
git add valid.c
git commit -m "Apply patch"

./autogen.sh
make CC=wllvm CXX=wllvm++ CFLAGS="-static -O0 -g" CXXFLAGS="-static -O0 -g" -j10

cp ./xmllint ../patched
cp ./valid.c ../patched
pushd ../patched
  cp ../exploit .
  cp ../Makefile .
popd